/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <cmath>
#include <limits>
#include <vector>

#include "NaxxSpellIds.h"
#include "Playerbots.h"

namespace
{
bool IsNothAdd(Unit const* unit)
{
    if (!unit)
        return false;

    switch (unit->GetEntry())
    {
        case NaxxSpellIds::NothPlaguedWarriorEntry:
        case NaxxSpellIds::NothPlaguedChampionEntry:
        case NaxxSpellIds::NothPlaguedGuardianEntry:
            return true;
        default:
            return false;
    }
}

uint32 GetNothAddPriority(Unit const* unit)
{
    if (!unit)
        return 0;

    switch (unit->GetEntry())
    {
        case NaxxSpellIds::NothPlaguedChampionEntry:
            return 30;
        case NaxxSpellIds::NothPlaguedGuardianEntry:
            return 20;
        case NaxxSpellIds::NothPlaguedWarriorEntry:
            return 10;
        default:
            return 0;
    }
}

bool IsControlledByRole(PlayerbotAI* botAI, Unit* add, bool assistTank)
{
    if (!botAI || !add || !add->GetVictim())
        return false;

    Player* victim = add->GetVictim()->ToPlayer();
    if (!victim)
        return false;

    return assistTank ? botAI->IsAssistTank(victim) : botAI->IsMainTank(victim);
}
}  // namespace

bool NothChooseTargetAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    GuidVector const attackers = AI_VALUE(GuidVector, "attackers");
    Unit* boss = nullptr;
    std::vector<Unit*> adds;

    for (ObjectGuid const& guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (botAI->EqualLowercaseName(unit->GetName(), "noth the plaguebringer"))
            boss = unit;
        else if (IsNothAdd(unit))
            adds.push_back(unit);
    }

    Unit* target = nullptr;
    bool const mainTank = botAI->IsMainTank(bot);
    bool const assistTank = botAI->IsAssistTank(bot);
    bool const hasAssistTank = helper.GetAliveAssistTank() != nullptr;

    if (assistTank || (mainTank && !hasAssistTank))
    {
        // Tanks first pick an add that is not already controlled by their assigned tank role.
        for (Unit* add : adds)
        {
            if (!IsControlledByRole(botAI, add, assistTank))
            {
                if (!target || GetNothAddPriority(add) > GetNothAddPriority(target) ||
                    (GetNothAddPriority(add) == GetNothAddPriority(target) && bot->GetDistance(add) < bot->GetDistance(target)))
                {
                    target = add;
                }
            }
        }

        if (!target)
        {
            for (Unit* add : adds)
            {
                if (!target || GetNothAddPriority(add) > GetNothAddPriority(target) ||
                    (GetNothAddPriority(add) == GetNothAddPriority(target) && bot->GetDistance(add) < bot->GetDistance(target)))
                {
                    target = add;
                }
            }
        }

        // During the ground phase the main tank returns to Noth after loose adds are controlled.
        if (!target && mainTank && !helper.IsBalconyPhase())
            target = boss;
    }
    else if (helper.IsBalconyPhase())
    {
        // Raid DPS burn balcony adds in order of danger.
        for (Unit* add : adds)
        {
            if (!target || GetNothAddPriority(add) > GetNothAddPriority(target) ||
                (GetNothAddPriority(add) == GetNothAddPriority(target) && add->GetHealthPct() < target->GetHealthPct()))
            {
                target = add;
            }
        }
    }
    else
    {
        target = boss;
    }

    if (!target || AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target);
}

bool NothPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    GuidVector const attackers = AI_VALUE(GuidVector, "attackers");

    if (botAI->IsAssistTank(bot))
    {
        Unit* looseAdd = nullptr;
        for (ObjectGuid const& guid : attackers)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (!unit || !unit->IsAlive() || !IsNothAdd(unit) || IsControlledByRole(botAI, unit, true))
                continue;

            if (!looseAdd || GetNothAddPriority(unit) > GetNothAddPriority(looseAdd) ||
                (GetNothAddPriority(unit) == GetNothAddPriority(looseAdd) && bot->GetDistance(unit) < bot->GetDistance(looseAdd)))
            {
                looseAdd = unit;
            }
        }

        if (looseAdd && bot->GetDistance(looseAdd) > 5.0f)
            return MoveNear(looseAdd, 4.0f, MovementPriority::MOVEMENT_COMBAT);

        // Keep controlled adds within healing range of the raid instead of dragging them to the room edge.
        Unit* currentTarget = AI_VALUE(Unit*, "current target");
        if (currentTarget && IsNothAdd(currentTarget) && AI_VALUE2(bool, "has aggro", "current target"))
        {
            Unit* nearestHealer = nullptr;
            float nearestHealerDistance = std::numeric_limits<float>::max();
            GuidVector const members = AI_VALUE(GuidVector, "group members");
            for (ObjectGuid const& guid : members)
            {
                Unit* member = botAI->GetUnit(guid);
                Player* player = member ? member->ToPlayer() : nullptr;
                if (!player || !player->IsAlive() || !botAI->IsHeal(player))
                    continue;

                float const distance = bot->GetDistance(player);
                if (distance < nearestHealerDistance)
                {
                    nearestHealer = player;
                    nearestHealerDistance = distance;
                }
            }

            if (nearestHealer && nearestHealerDistance > 30.0f)
                return MoveNear(nearestHealer, 25.0f, MovementPriority::MOVEMENT_COMBAT);
        }

        return false;
    }

    if (!helper.IsBalconyPhase() || !botAI->IsRanged(bot))
        return false;

    Unit* nearestChampion = nullptr;
    float nearestDistance = std::numeric_limits<float>::max();
    for (ObjectGuid const& guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->GetEntry() != NaxxSpellIds::NothPlaguedChampionEntry)
            continue;

        float const distance = bot->GetDistance(unit);
        if (distance < nearestDistance)
        {
            nearestChampion = unit;
            nearestDistance = distance;
        }
    }

    if (!nearestChampion || nearestDistance >= 25.0f)
        return false;

    float const angle = nearestChampion->GetAngle(bot);
    float const x = nearestChampion->GetPositionX() + std::cos(angle) * 25.0f;
    float const y = nearestChampion->GetPositionY() + std::sin(angle) * 25.0f;
    return MoveTo(NAXX_MAP_ID, x, y, bot->GetPositionZ(), false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT);
}
