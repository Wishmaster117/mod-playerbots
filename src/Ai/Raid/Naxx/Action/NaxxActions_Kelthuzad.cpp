/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <algorithm>
#include <cmath>

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

bool KelthuzadChooseTargetAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    GuidVector const targets = context->GetValue<GuidVector>("possible targets")->Get();
    Unit* soldier = nullptr;
    Unit* weaver = nullptr;
    Unit* abomination = nullptr;
    Unit* kelthuzad = helper.GetBoss();

    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        float const centerDistance = unit->GetDistance2d(helper.center.first, helper.center.second);
        if (centerDistance > KelthuzadBossHelper::ROOM_MAX_RADIUS + 4.0f)
            continue;

        switch (unit->GetEntry())
        {
            case NaxxSpellIds::KelthuzadSoldierEntry:
                if (!soldier || centerDistance < soldier->GetDistance2d(helper.center.first, helper.center.second))
                    soldier = unit;
                break;
            case NaxxSpellIds::KelthuzadAbominationEntry:
                if (!abomination || centerDistance < abomination->GetDistance2d(helper.center.first, helper.center.second))
                    abomination = unit;
                break;
            case NaxxSpellIds::KelthuzadSoulWeaverEntry:
                if (!weaver || centerDistance < weaver->GetDistance2d(helper.center.first, helper.center.second))
                    weaver = unit;
                break;
            default:
                break;
        }
    }

    bool const isOffTank = botAI->IsTank(bot) && !botAI->IsMainTank(bot) && botAI->IsAssistTank(bot);
    std::vector<Unit*> guardians = helper.GetGuardians();
    Unit* guardian = isOffTank ? helper.GetGuardianToPickup(bot) : nullptr;
    Unit* target = nullptr;

    if (isOffTank && !guardians.empty())
    {
        target = guardian;
    }
    else if (helper.IsPhaseOne())
    {
        if (botAI->IsTank(bot))
            target = abomination ? abomination : (weaver ? weaver : soldier);
        else if (botAI->IsRanged(bot))
            target = botAI->GetRangedDpsIndex(bot) <= 1 ? (soldier ? soldier : (weaver ? weaver : abomination))
                                                       : (weaver ? weaver : (soldier ? soldier : abomination));
        else
            target = abomination ? abomination : (soldier ? soldier : weaver);
    }
    else
    {
        bool const remainingPhaseOneAdds = soldier || weaver || abomination;
        if (!botAI->IsHeal(bot) && remainingPhaseOneAdds)
        {
            if (botAI->IsRanged(bot))
                target = weaver ? weaver : (soldier ? soldier : abomination);
            else
                target = abomination ? abomination : (soldier ? soldier : weaver);
        }
        else
        {
            target = kelthuzad;
        }
    }

    if (!target)
        return false;

    if (!botAI->IsRanged(bot) && !helper.IsWithinRoom(target, KelthuzadBossHelper::ROOM_MAX_RADIUS + 2.0f))
        return false;

    if (AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target, target == kelthuzad);
}

bool KelthuzadPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    if (helper.IsPhaseOne())
    {
        if (botAI->IsTank(bot))
        {
            Unit* target = AI_VALUE(Unit*, "current target");
            if (target && helper.IsWithinRoom(target, KelthuzadBossHelper::PHASE1_TANK_MAX_RADIUS) &&
                bot->GetDistance2d(target) > 4.0f)
            {
                return MoveNear(target, 3.0f, MovementPriority::MOVEMENT_COMBAT);
            }

            if (bot->GetDistance2d(helper.center.first, helper.center.second) > KelthuzadBossHelper::PHASE1_TANK_HOLD_RADIUS)
            {
                return MoveInside(NAXX_MAP_ID, helper.center.first, helper.center.second, bot->GetPositionZ(),
                                  KelthuzadBossHelper::PHASE1_TANK_HOLD_RADIUS, MovementPriority::MOVEMENT_COMBAT);
            }
            return false;
        }

        if (bot->GetDistance2d(helper.center.first, helper.center.second) > 20.0f)
        {
            return MoveInside(NAXX_MAP_ID, helper.center.first, helper.center.second, bot->GetPositionZ(), 3.0f,
                              MovementPriority::MOVEMENT_COMBAT);
        }

        Unit* target = AI_VALUE(Unit*, "current target");
        if (!botAI->IsRanged(bot) && target && helper.IsWithinRoom(target, 20.0f) && bot->GetDistance2d(target) > 4.0f)
            return MoveNear(target, 3.0f, MovementPriority::MOVEMENT_COMBAT);

        return false;
    }

    if (!helper.IsPhaseTwo())
        return false;

    if (helper.HasChains(bot))
    {
        bot->AttackStop();
        bot->StopMoving();
        return true;
    }

    if (helper.HasDetonateMana(bot))
    {
        float dx = bot->GetPositionX() - helper.center.first;
        float dy = bot->GetPositionY() - helper.center.second;
        float length = std::sqrt(dx * dx + dy * dy);
        if (length < 0.1f)
        {
            float const angle = float(botAI->GetGroupSlotIndex(bot)) * (float(M_PI) / 8.0f);
            dx = std::cos(angle);
            dy = std::sin(angle);
            length = 1.0f;
        }

        float const radius = KelthuzadBossHelper::DETONATE_MAX_RADIUS;
        float x = helper.center.first + dx / length * radius;
        float y = helper.center.second + dy / length * radius;
        helper.ClampToRoom(x, y, KelthuzadBossHelper::DETONATE_MIN_RADIUS, KelthuzadBossHelper::DETONATE_MAX_RADIUS);

        if (bot->GetDistance2d(x, y) <= 1.5f)
            return false;

        return MoveTo(NAXX_MAP_ID, x, y, bot->GetPositionZ(), false, false, false, false,
                      MovementPriority::MOVEMENT_FORCED);
    }

    if (Player* frostBlastTarget = helper.GetPlayerWithAura(NaxxSpellIds::FrostBlast))
    {
        if (frostBlastTarget == bot)
        {
            bot->StopMoving();
            return false;
        }

        if (bot->GetDistance2d(frostBlastTarget) < 11.0f)
        {
            float dx = bot->GetPositionX() - frostBlastTarget->GetPositionX();
            float dy = bot->GetPositionY() - frostBlastTarget->GetPositionY();
            float length = std::sqrt(dx * dx + dy * dy);
            if (length < 0.1f)
            {
                float const angle = float(botAI->GetGroupSlotIndex(bot)) * (float(M_PI) / 8.0f);
                dx = std::cos(angle);
                dy = std::sin(angle);
                length = 1.0f;
            }

            float x = frostBlastTarget->GetPositionX() + dx / length * 12.0f;
            float y = frostBlastTarget->GetPositionY() + dy / length * 12.0f;
            helper.ClampToRoom(x, y);
            return MoveTo(NAXX_MAP_ID, x, y, bot->GetPositionZ(), false, false, false, false,
                          MovementPriority::MOVEMENT_FORCED);
        }
    }

    if (Unit* fissure = helper.GetAnyShadowFissure())
    {
        if (bot->GetDistance2d(fissure) < 11.0f)
        {
            float dx = bot->GetPositionX() - fissure->GetPositionX();
            float dy = bot->GetPositionY() - fissure->GetPositionY();
            float length = std::sqrt(dx * dx + dy * dy);
            if (length < 0.1f)
            {
                dx = bot->GetPositionX() - helper.center.first;
                dy = bot->GetPositionY() - helper.center.second;
                length = std::max(0.1f, std::sqrt(dx * dx + dy * dy));
            }

            float x = fissure->GetPositionX() + dx / length * 12.0f;
            float y = fissure->GetPositionY() + dy / length * 12.0f;
            helper.ClampToRoom(x, y);
            return MoveTo(NAXX_MAP_ID, x, y, bot->GetPositionZ(), false, false, false, false,
                          MovementPriority::MOVEMENT_FORCED);
        }
    }

    if (botAI->IsMainTank(bot))
    {
        if (!AI_VALUE2(bool, "has aggro", "current target"))
            return false;

        std::pair<float, float> const hold = helper.GetMainTankHoldPosition();
        if (bot->GetDistance2d(hold.first, hold.second) <= 2.0f)
            return false;

        return MoveTo(NAXX_MAP_ID, hold.first, hold.second, bot->GetPositionZ(), false, false, false, false,
                      MovementPriority::MOVEMENT_COMBAT);
    }

    bool const isOffTank = botAI->IsTank(bot) && !botAI->IsMainTank(bot) && botAI->IsAssistTank(bot);
    if (isOffTank)
    {
        std::vector<Unit*> guardians = helper.GetGuardians();
        if (!guardians.empty())
        {
            Unit* pickup = helper.GetGuardianToPickup(bot);
            if (pickup && pickup->GetVictim() != bot && bot->GetDistance2d(pickup) > 4.0f)
                return MoveNear(pickup, 3.0f, MovementPriority::MOVEMENT_COMBAT);

            if (helper.AllGuardiansOnAssistTank(bot))
            {
                std::pair<float, float> const hold = helper.GetAssistTankHoldPosition();
                if (bot->GetDistance2d(hold.first, hold.second) > 2.0f)
                {
                    return MoveTo(NAXX_MAP_ID, hold.first, hold.second, bot->GetPositionZ(), false, false, false, false,
                                  MovementPriority::MOVEMENT_COMBAT);
                }
            }
        }
        return false;
    }

    if (botAI->IsRanged(bot))
    {
        float x = 0.0f;
        float y = 0.0f;
        helper.ComputeRangedSpreadPosition(botAI->GetRangedIndex(bot), std::max<uint32>(1, helper.GetRangedCount()), x, y);
        if (bot->GetDistance2d(x, y) <= 2.0f)
            return false;

        return MoveTo(NAXX_MAP_ID, x, y, bot->GetPositionZ(), false, false, false, false,
                      MovementPriority::MOVEMENT_COMBAT);
    }

    return false;
}
