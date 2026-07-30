/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <cmath>
#include <limits>

#include "Playerbots.h"

namespace
{
    constexpr float MeleeBehindDistance = 4.5f;
    constexpr float RangedBehindDistance = 15.0f;
    constexpr float RangedSideOffset = 6.0f;
    constexpr float HealerBehindDistance = 22.0f;
    constexpr float SporeTankDangerRadius = 10.0f;
    constexpr float SporePullDistance = 12.0f;

    uint8 GetRangedSide(Player* player)
    {
        return player ? player->GetSubGroup() % 2 : 0;
    }

    void GetBehindPosition(Unit* boss, float distance, float sideOffset, float& x, float& y, float& z)
    {
        float const orientation = boss->GetOrientation();
        float const backX = -std::cos(orientation);
        float const backY = -std::sin(orientation);
        float const rightX = -backY;
        float const rightY = backX;

        x = boss->GetPositionX() + backX * distance + rightX * sideOffset;
        y = boss->GetPositionY() + backY * distance + rightY * sideOffset;
        z = boss->GetPositionZ();
    }

    void GetRangedPackPosition(Unit* boss, uint8 side, float& x, float& y, float& z)
    {
        float const offset = side == 0 ? -RangedSideOffset : RangedSideOffset;
        GetBehindPosition(boss, RangedBehindDistance, offset, x, y, z);
    }

    uint8 GetSporeSide(Unit* boss, Unit* spore)
    {
        float const orientation = boss->GetOrientation();
        float const backX = -std::cos(orientation);
        float const backY = -std::sin(orientation);
        float const rightX = -backY;
        float const rightY = backX;
        float const deltaX = spore->GetPositionX() - boss->GetPositionX();
        float const deltaY = spore->GetPositionY() - boss->GetPositionY();
        return deltaX * rightX + deltaY * rightY < 0.0f ? 0 : 1;
    }

    ObjectGuid SelectSporeRunner(Player* bot, PlayerbotAI* botAI, uint8 side)
    {
        Group* group = bot ? bot->GetGroup() : nullptr;
        if (!group)
            return ObjectGuid::Empty;

        ObjectGuid selected = ObjectGuid::Empty;
        for (GroupReference* reference = group->GetFirstMember(); reference; reference = reference->next())
        {
            Player* member = reference->GetSource();
            if (!member || !member->IsAlive() || member->GetMapId() != bot->GetMapId())
                continue;

            if (GetRangedSide(member) != side || !botAI->IsRanged(member) || botAI->IsHeal(member) || botAI->IsTank(member))
                continue;

            if (selected.IsEmpty() || member->GetGUID() < selected)
                selected = member->GetGUID();
        }

        return selected;
    }

    bool IsSporeRunner(Player* bot, PlayerbotAI* botAI, uint8 side)
    {
        ObjectGuid const selected = SelectSporeRunner(bot, botAI, side);
        return !selected.IsEmpty() && selected == bot->GetGUID();
    }

    Unit* SelectAssignedSpore(Player* bot, PlayerbotAI* botAI, Unit* boss, Unit* mainTank,
                              GuidVector const& candidates, uint8 side)
    {
        Unit* tankDangerSpore = nullptr;
        Unit* sideSpore = nullptr;
        Unit* fallbackSpore = nullptr;
        float sideDistance = std::numeric_limits<float>::max();
        float fallbackDistance = std::numeric_limits<float>::max();
        float tankDistance = std::numeric_limits<float>::max();

        for (ObjectGuid const& guid : candidates)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (!unit || !unit->IsAlive() || !botAI->EqualLowercaseName(unit->GetName(), "spore"))
                continue;

            float const distance = bot->GetDistance(unit);
            if (distance < fallbackDistance)
            {
                fallbackSpore = unit;
                fallbackDistance = distance;
            }

            if (GetSporeSide(boss, unit) == side && distance < sideDistance)
            {
                sideSpore = unit;
                sideDistance = distance;
            }

            if (mainTank && mainTank->IsAlive())
            {
                float const distanceToTank = mainTank->GetDistance(unit);
                if (distanceToTank <= SporeTankDangerRadius && distanceToTank < tankDistance)
                {
                    tankDangerSpore = unit;
                    tankDistance = distanceToTank;
                }
            }
        }

        if (tankDangerSpore)
            return tankDangerSpore;

        return sideSpore ? sideSpore : fallbackSpore;
    }
}  // namespace

bool LoathebPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    Unit* boss = helper.GetBoss();
    if (!boss)
        return false;

    if (botAI->IsTank(bot))
    {
        if (!AI_VALUE2(bool, "has aggro", "boss target"))
            return false;

        return MoveTo(NAXX_MAP_ID, helper.mainTankPos.first, helper.mainTankPos.second, boss->GetPositionZ(), false, false, false,
                      false, MovementPriority::MOVEMENT_COMBAT);
    }

    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetZ = boss->GetPositionZ();

    if (botAI->IsHeal(bot))
    {
        GetBehindPosition(boss, HealerBehindDistance, 0.0f, targetX, targetY, targetZ);
    }
    else if (botAI->IsMelee(bot))
    {
        GetBehindPosition(boss, MeleeBehindDistance, 0.0f, targetX, targetY, targetZ);
    }
    else
    {
        uint8 const side = GetRangedSide(bot);
        GetRangedPackPosition(boss, side, targetX, targetY, targetZ);

        if (IsSporeRunner(bot, botAI, side))
        {
            GuidVector const candidates = context->GetValue<GuidVector>("attackers")->Get();
            Unit* mainTank = AI_VALUE(Unit*, "main tank");
            Unit* spore = SelectAssignedSpore(bot, botAI, boss, mainTank, candidates, side);
            if (spore && spore->GetVictim() != bot && bot->GetDistance(spore) > SporePullDistance)
            {
                return MoveNear(spore, 8.0f, MovementPriority::MOVEMENT_COMBAT);
            }
        }
    }

    return MoveInside(NAXX_MAP_ID, targetX, targetY, targetZ, 1.5f, MovementPriority::MOVEMENT_COMBAT);
}

bool LoathebChooseTargetAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    Unit* boss = helper.GetBoss();
    if (!boss)
        return false;

    Unit* target = boss;
    if (botAI->IsRanged(bot) && !botAI->IsHeal(bot) && !botAI->IsTank(bot))
    {
        uint8 const side = GetRangedSide(bot);
        if (IsSporeRunner(bot, botAI, side))
        {
            GuidVector const candidates = context->GetValue<GuidVector>("attackers")->Get();
            Unit* mainTank = AI_VALUE(Unit*, "main tank");
            if (Unit* spore = SelectAssignedSpore(bot, botAI, boss, mainTank, candidates, side))
                target = spore;
        }
    }

    if (AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target);
}
