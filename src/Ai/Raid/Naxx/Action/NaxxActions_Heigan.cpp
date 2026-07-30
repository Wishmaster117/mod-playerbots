/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include "NaxxSpellIds.h"
#include "Playerbots.h"
#include "Timer.h"

namespace
{
    constexpr uint32 SlowDanceFirstEruptionMs = 15000;
    constexpr uint32 SlowDancePeriodMs = 10000;
    constexpr uint32 FastDanceFirstEruptionMs = 7000;
    constexpr uint32 FastDancePeriodMs = 4000;
    constexpr uint32 MoveAfterEruptionDelayMs = 250;
    constexpr float PlatformPhaseMinZ = 270.0f;
    constexpr float PlatformDetectionRadius = 14.0f;
}

bool HeiganDanceAction::IsPlatformPhase(Unit* boss) const
{
    if (!boss)
        return false;

    return boss->GetPositionZ() >= PlatformPhaseMinZ &&
           boss->IsWithinDist2d(platform.first, platform.second, PlatformDetectionRadius);
}

void HeiganDanceAction::ResetPhase(bool nextPlatformPhase, uint32 now)
{
    initialized = true;
    platformPhase = nextPlatformPhase;
    phaseStartMs = now;
    processedEruptions = 0;

    // AzerothCore starts every Heigan phase with section 3 as the safe section,
    // then progresses 3 -> 2 -> 1 -> 0 -> 1 -> 2 -> 3.
    currentSafeSection = 3;
    direction = -1;
}

void HeiganDanceAction::AdvanceSafeSection()
{
    if (currentSafeSection == 0)
        direction = 1;
    else if (currentSafeSection == 3)
        direction = -1;

    int32 next = static_cast<int32>(currentSafeSection) + direction;
    if (next < 0)
        next = 0;
    else if (next > 3)
        next = 3;

    currentSafeSection = static_cast<uint8>(next);
}

bool HeiganDanceAction::UpdateDanceState()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!boss || !boss->IsAlive())
        return false;

    uint32 now = getMSTime();
    bool nextPlatformPhase = IsPlatformPhase(boss);
    if (!initialized || nextPlatformPhase != platformPhase)
        ResetPhase(nextPlatformPhase, now);

    uint32 firstEruption = platformPhase ? FastDanceFirstEruptionMs : SlowDanceFirstEruptionMs;
    uint32 period = platformPhase ? FastDancePeriodMs : SlowDancePeriodMs;
    uint32 elapsed = now - phaseStartMs;
    uint32 completedEruptions = 0;
    uint32 firstMovement = firstEruption + MoveAfterEruptionDelayMs;

    if (elapsed >= firstMovement)
        completedEruptions = 1u + (elapsed - firstMovement) / period;

    while (processedEruptions < completedEruptions)
    {
        AdvanceSafeSection();
        ++processedEruptions;
    }

    return true;
}

bool HeiganDanceMeleeAction::Execute(Event /*event*/)
{
    if (!UpdateDanceState())
        return false;

    if (!platformPhase && botAI->IsMainTank(bot) && !AI_VALUE2(bool, "has aggro", "boss target"))
        return false;

    float radius = botAI->IsMainTank(bot) ? 0.0f : 1.5f;
    return MoveInside(bot->GetMapId(), waypoints[currentSafeSection].first, waypoints[currentSafeSection].second,
                      arenaZ, radius, MovementPriority::MOVEMENT_COMBAT);
}

bool HeiganDanceRangedAction::Execute(Event /*event*/)
{
    if (!UpdateDanceState())
        return false;

    if (!platformPhase)
    {
        if (bot->IsWithinDist2d(platform.first, platform.second, 3.0f) && bot->GetPositionZ() >= PlatformPhaseMinZ)
            return false;

        if (MoveTo(bot->GetMapId(), platform.first, platform.second, platformZ, false, false, false, false,
                   MovementPriority::MOVEMENT_COMBAT))
        {
            return true;
        }

        return MoveInside(bot->GetMapId(), platform.first, platform.second, platformZ, 2.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }

    botAI->InterruptSpell();
    return MoveInside(bot->GetMapId(), waypoints[currentSafeSection].first, waypoints[currentSafeSection].second,
                      arenaZ, 1.5f, MovementPriority::MOVEMENT_COMBAT);
}

bool HeiganDispelDecrepitFeverAction::IsDiseaseDispeller() const
{
    switch (bot->getClass())
    {
        case CLASS_PALADIN:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            return true;
        default:
            return false;
    }
}

Unit* HeiganDispelDecrepitFeverAction::FindTarget() const
{
    float range = botAI->GetRange("heal");
    Unit* bestTank = nullptr;
    Unit* bestOther = nullptr;
    float bestOtherHealth = 101.0f;

    auto consider = [&](Player* member)
    {
        if (!member || !member->IsAlive() || member->GetMapId() != bot->GetMapId())
            return;

        if (!member->HasAura(NaxxSpellIds::DecrepitFever))
            return;

        if (!bot->IsWithinDistInMap(member, range))
            return;

        if (botAI->IsMainTank(member))
        {
            bestTank = member;
            return;
        }

        if (botAI->IsTank(member) && !bestTank)
        {
            bestTank = member;
            return;
        }

        if (!bestOther || member->GetHealthPct() < bestOtherHealth)
        {
            bestOther = member;
            bestOtherHealth = member->GetHealthPct();
        }
    };

    if (Group* group = bot->GetGroup())
    {
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            consider(ref->GetSource());
    }
    else
    {
        consider(bot);
    }

    return bestTank ? bestTank : bestOther;
}

bool HeiganDispelDecrepitFeverAction::isUseful()
{
    if (!IsDiseaseDispeller())
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!boss || !boss->IsAlive())
        return false;

    // Decrepit Fever is a slow-phase mechanic. Never let a dispel interrupt the fast dance.
    if (boss->GetPositionZ() >= PlatformPhaseMinZ)
        return false;

    return FindTarget() != nullptr;
}

bool HeiganDispelDecrepitFeverAction::Execute(Event /*event*/)
{
    Unit* target = FindTarget();
    if (!target)
        return false;

    switch (bot->getClass())
    {
        case CLASS_PALADIN:
            if (botAI->CanCastSpell("cleanse", target) && botAI->CastSpell("cleanse", target))
                return true;
            return botAI->CanCastSpell("purify", target) && botAI->CastSpell("purify", target);
        case CLASS_PRIEST:
            if (botAI->CanCastSpell("abolish disease", target) && botAI->CastSpell("abolish disease", target))
                return true;
            return botAI->CanCastSpell("cure disease", target) && botAI->CastSpell("cure disease", target);
        case CLASS_SHAMAN:
            if (botAI->CanCastSpell("cleanse spirit", target) && botAI->CastSpell("cleanse spirit", target))
                return true;
            return botAI->CanCastSpell("cure toxins", target) && botAI->CastSpell("cure toxins", target);
        default:
            return false;
    }
}
