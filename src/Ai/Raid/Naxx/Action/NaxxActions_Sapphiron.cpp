/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "NaxxBossHelper.h"
#include "NaxxSpellIds.h"

namespace
{
    constexpr float MeleeSideDistance = 6.0f;
    constexpr float HealerSideDistance = 26.0f;
    constexpr float RangedSideDistance = 32.0f;
    constexpr float GroundPositionTolerance = 2.5f;
    constexpr float ShelterDistance = 8.0f;
    constexpr float ShelterTolerance = 0.75f;
    constexpr float ShelterLateralOffset = 1.0f;

    float GetGroundDistance(PlayerbotAI* botAI, Player* bot)
    {
        if (botAI->IsHeal(bot))
            return HealerSideDistance;
        if (botAI->IsRanged(bot))
            return RangedSideDistance;
        return MeleeSideDistance;
    }
}

bool SapphironGroundPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI() || !helper.IsPhaseGround())
        return false;

    std::vector<float> chillDestination;
    if (helper.FindPosToAvoidChill(chillDestination))
    {
        return MoveTo(NAXX_MAP_ID, chillDestination[0], chillDestination[1], chillDestination[2], false, false, false,
                      false, MovementPriority::MOVEMENT_COMBAT);
    }

    Unit* boss = helper.GetBoss();
    if (!boss)
        return false;

    if (botAI->IsMainTank(bot))
    {
        if (!AI_VALUE2(bool, "has aggro", "current target"))
            return false;

        if (bot->GetDistance2d(helper.mainTankPos.first, helper.mainTankPos.second) <= GroundPositionTolerance)
            return false;

        return MoveTo(NAXX_MAP_ID, helper.mainTankPos.first, helper.mainTankPos.second, helper.GENERIC_HEIGHT, false, false,
                      false, false, MovementPriority::MOVEMENT_COMBAT);
    }

    int32 const slotIndex = std::max<int32>(0, botAI->GetGroupSlotIndex(bot));
    float const sideSign = slotIndex % 2 == 0 ? 1.0f : -1.0f;
    float const arcOffset = (static_cast<float>((slotIndex / 2) % 5) - 2.0f) * 0.07f;
    float const angle = boss->GetOrientation() + sideSign * (static_cast<float>(M_PI) / 2.0f + arcOffset);
    float const distance = GetGroundDistance(botAI, bot);
    float const x = boss->GetPositionX() + std::cos(angle) * distance;
    float const y = boss->GetPositionY() + std::sin(angle) * distance;
    float const z = boss->GetPositionZ();

    if (bot->GetDistance2d(x, y) <= GroundPositionTolerance)
        return false;

    return MoveTo(NAXX_MAP_ID, x, y, z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
}

bool SapphironFlightPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI() || !helper.IsPhaseFlight())
        return false;

    if (helper.IsIceboltTarget(bot))
    {
        botAI->InterruptSpell();
        bot->StopMoving();
        return false;
    }

    if (helper.WaitForExplosion())
        return MoveToNearestIcebolt();

    std::vector<float> chillDestination;
    if (helper.FindPosToAvoidChill(chillDestination))
    {
        return MoveTo(NAXX_MAP_ID, chillDestination[0], chillDestination[1], chillDestination[2], false, false, false,
                      false, MovementPriority::MOVEMENT_COMBAT);
    }

    return false;
}

bool SapphironFlightPositionAction::MoveToNearestIcebolt()
{
    Unit* boss = helper.GetBoss();
    if (!boss)
        return false;

    std::vector<Player*> icebolts;
    helper.GetIceboltTargets(icebolts);
    if (icebolts.empty())
        return false;

    std::sort(icebolts.begin(), icebolts.end(), [](Player const* left, Player const* right)
    {
        return left->GetGUID().GetRawValue() < right->GetGUID().GetRawValue();
    });

    Player* nearest = nullptr;
    float nearestDistance = std::numeric_limits<float>::max();
    for (Player* icebolt : icebolts)
    {
        float const distance = bot->GetDistance(icebolt);
        if (!nearest || distance < nearestDistance)
        {
            nearest = icebolt;
            nearestDistance = distance;
        }
    }

    int32 const slotIndex = std::max<int32>(0, botAI->GetGroupSlotIndex(bot));
    Player* assigned = icebolts[static_cast<size_t>(slotIndex) % icebolts.size()];
    if (!assigned || bot->GetDistance(assigned) > nearestDistance + 18.0f)
        assigned = nearest;
    if (!assigned)
        return false;

    float const awayAngle = boss->GetAngle(assigned);
    float const lateralSign = slotIndex % 2 == 0 ? 1.0f : -1.0f;
    float const lateralAngle = awayAngle + static_cast<float>(M_PI) / 2.0f;
    float const x = assigned->GetPositionX() + std::cos(awayAngle) * ShelterDistance +
                    std::cos(lateralAngle) * ShelterLateralOffset * lateralSign;
    float const y = assigned->GetPositionY() + std::sin(awayAngle) * ShelterDistance +
                    std::sin(lateralAngle) * ShelterLateralOffset * lateralSign;
    float const z = assigned->GetPositionZ();

    bool const alreadySheltered = assigned->IsInBetween(boss, bot, 2.0f) && assigned->GetDistance2d(bot) <= 10.0f;
    if (alreadySheltered && bot->GetDistance2d(x, y) <= ShelterTolerance)
    {
        botAI->InterruptSpell();
        bot->StopMoving();
        return true;
    }

    botAI->InterruptSpell();
    return MoveTo(NAXX_MAP_ID, x, y, z, false, false, false, true, MovementPriority::MOVEMENT_FORCED);
}
