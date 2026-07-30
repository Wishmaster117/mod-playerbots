/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "NaxxSpellIds.h"
#include "Playerbots.h"

namespace
{
    constexpr float RoomCenterX = 3281.23f;
    constexpr float RoomCenterY = -3310.38f;
    constexpr float TankRingRadius = 35.0f;
    constexpr float InjectionRingRadius = 43.0f;
    constexpr float MaxRegularPositionRadius = 37.0f;
    constexpr float TwoPi = 2.0f * M_PI;

    bool HasMutatingInjection(PlayerbotAI* botAI, Unit* unit)
    {
        return unit && (NaxxSpellIds::HasAnyAura(unit, {NaxxSpellIds::MutatingInjection}) ||
                        botAI->HasAura("mutating injection", unit, false, false, -1, true));
    }

    bool IsPoisonCloud(PlayerbotAI* botAI, Unit* unit)
    {
        if (!unit || !unit->IsAlive())
            return false;

        return unit->HasAura(NaxxSpellIds::PoisonCloudDamageAura) ||
               botAI->EqualLowercaseName(unit->GetName(), "poison cloud");
    }

    void ClampToRoom(float& x, float& y, float maxRadius)
    {
        float const dx = x - RoomCenterX;
        float const dy = y - RoomCenterY;
        float const distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= maxRadius || distance < 0.01f)
            return;

        float const scale = maxRadius / distance;
        x = RoomCenterX + dx * scale;
        y = RoomCenterY + dy * scale;
    }
}

bool GrobbulusInjectionPositionAction::MoveToSafeDropPosition()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss || !boss->IsAlive() || !HasMutatingInjection(botAI, bot))
        return false;

    std::vector<Unit*> clouds;
    GuidVector const triggers = context->GetValue<GuidVector>("nearest triggers")->Get();
    for (ObjectGuid const& guid : triggers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (IsPoisonCloud(botAI, unit))
            clouds.push_back(unit);
    }

    float baseAngle = std::atan2(bot->GetPositionY() - RoomCenterY, bot->GetPositionX() - RoomCenterX);
    if (bot->GetDistance2d(RoomCenterX, RoomCenterY) < 3.0f)
    {
        baseAngle = std::atan2(boss->GetPositionY() - RoomCenterY, boss->GetPositionX() - RoomCenterX) + M_PI;
    }

    int32 const groupSlot = std::max<int32>(0, botAI->GetGroupSlotIndex(bot));
    baseAngle += float((groupSlot % 3) - 1) * 0.08f;

    float bestX = bot->GetPositionX();
    float bestY = bot->GetPositionY();
    float bestScore = -std::numeric_limits<float>::max();

    for (uint32 index = 0; index < 12; ++index)
    {
        float const angle = baseAngle + TwoPi * float(index) / 12.0f;
        float const candidateX = RoomCenterX + std::cos(angle) * InjectionRingRadius;
        float const candidateY = RoomCenterY + std::sin(angle) * InjectionRingRadius;
        float const bossDistance = boss->GetDistance2d(candidateX, candidateY);
        if (bossDistance < 18.0f)
            continue;

        float cloudClearance = 60.0f;
        for (Unit* cloud : clouds)
        {
            cloudClearance = std::min(cloudClearance, cloud->GetDistance2d(candidateX, candidateY));
        }

        float const travelDistance = bot->GetDistance2d(candidateX, candidateY);
        float const score = cloudClearance * 4.0f + bossDistance * 0.5f - travelDistance;
        if (score > bestScore)
        {
            bestScore = score;
            bestX = candidateX;
            bestY = candidateY;
        }
    }

    if (bot->GetDistance2d(bestX, bestY) <= 2.0f)
    {
        bot->StopMoving();
        return false;
    }

    botAI->InterruptSpell();
    return MoveTo(bot->GetMapId(), bestX, bestY, bot->GetPositionZ(), false, false, false, true,
                  MovementPriority::MOVEMENT_FORCED);
}

bool GrobbulusGoBehindAction::Execute(Event /*event*/)
{
    return MoveToSafeDropPosition();
}

bool GrobbulusMoveAwayAction::Execute(Event /*event*/)
{
    return MoveToSafeDropPosition();
}

bool GrobbulusPositionAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss || !boss->IsAlive() || (!bot->IsInCombat() && !boss->IsInCombat()) || HasMutatingInjection(botAI, bot))
        return false;

    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    if (!botAI->IsMainTank(bot) && currentTarget && currentTarget->IsAlive() &&
        currentTarget->GetEntry() == NaxxSpellIds::FalloutSlimeEntry)
    {
        // Let normal reach/attack actions pursue the assigned slime instead of dragging the bot back to formation.
        return false;
    }

    float targetX = bot->GetPositionX();
    float targetY = bot->GetPositionY();
    float tolerance = 2.5f;

    if (botAI->IsMainTank(bot))
    {
        if (!AI_VALUE2(bool, "has aggro", "boss target"))
            return false;

        float angle = std::atan2(bot->GetPositionY() - RoomCenterY, bot->GetPositionX() - RoomCenterX);
        if (bot->GetDistance2d(RoomCenterX, RoomCenterY) < 3.0f)
            angle = std::atan2(boss->GetPositionY() - RoomCenterY, boss->GetPositionX() - RoomCenterX);

        targetX = RoomCenterX + std::cos(angle) * TankRingRadius;
        targetY = RoomCenterY + std::sin(angle) * TankRingRadius;
        tolerance = 3.0f;
    }
    else if (botAI->IsMelee(bot) || botAI->IsTank(bot))
    {
        float const distance = botAI->IsTank(bot) ? 8.0f : 6.0f;
        float const angle = boss->GetOrientation() + M_PI;
        targetX = boss->GetPositionX() + std::cos(angle) * distance;
        targetY = boss->GetPositionY() + std::sin(angle) * distance;
        ClampToRoom(targetX, targetY, MaxRegularPositionRadius);
    }
    else
    {
        int32 const groupSlot = std::max<int32>(0, botAI->GetGroupSlotIndex(bot));
        float const angle = float(groupSlot) * 2.39996323f;
        float const radius = 8.0f + float(groupSlot % 2) * 4.0f;
        targetX = RoomCenterX + std::cos(angle) * radius;
        targetY = RoomCenterY + std::sin(angle) * radius;
    }

    if (bot->GetDistance2d(targetX, targetY) <= tolerance)
        return false;

    return MoveTo(bot->GetMapId(), targetX, targetY, bot->GetPositionZ(), false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT);
}

bool GrobbulusChooseTargetAction::Execute(Event /*event*/)
{
    if (botAI->IsHeal(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss || !boss->IsAlive())
        return false;

    std::vector<Unit*> slimes;
    auto collectSlimes = [&](GuidVector const& guids)
    {
        for (ObjectGuid const& guid : guids)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (!unit || !unit->IsAlive() || unit->GetEntry() != NaxxSpellIds::FalloutSlimeEntry)
                continue;

            if (std::find(slimes.begin(), slimes.end(), unit) == slimes.end())
                slimes.push_back(unit);
        }
    };

    collectSlimes(context->GetValue<GuidVector>("possible targets")->Get());
    collectSlimes(context->GetValue<GuidVector>("attackers")->Get());

    Unit* target = nullptr;
    if (botAI->IsMainTank(bot))
    {
        target = boss;
    }
    else if (botAI->IsTank(bot))
    {
        for (Unit* slime : slimes)
        {
            Player* victim = slime->GetVictim() ? slime->GetVictim()->ToPlayer() : nullptr;
            if (!victim || !botAI->IsTank(victim))
            {
                if (!target || bot->GetDistance2d(slime) < bot->GetDistance2d(target))
                    target = slime;
            }
        }

        if (!target && !slimes.empty())
            target = *std::min_element(slimes.begin(), slimes.end(), [this](Unit* left, Unit* right)
            {
                return bot->GetDistance2d(left) < bot->GetDistance2d(right);
            });
    }
    else if (!slimes.empty())
    {
        target = *std::min_element(slimes.begin(), slimes.end(), [this](Unit* left, Unit* right)
        {
            if (std::abs(left->GetHealthPct() - right->GetHealthPct()) > 0.1f)
                return left->GetHealthPct() < right->GetHealthPct();
            return bot->GetDistance2d(left) < bot->GetDistance2d(right);
        });
    }

    if (!target)
        target = boss;

    if (AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target, target == boss);
}

uint32 GrobbulusRotateAction::GetCurrWaypoint()
{
    uint32 const current = FindNearestWaypoint();
    if (clockwise)
        return (current + 1) % intervals;

    return (current + intervals - 1) % intervals;
}
