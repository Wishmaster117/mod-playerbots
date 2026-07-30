/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "SharedDefines.h"

bool GluthChooseTargetAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    GuidVector const candidates = context->GetValue<GuidVector>("possible targets")->Get();
    Unit* boss = helper.GetBoss();
    Unit* target = nullptr;
    std::vector<Unit*> zombies;

    for (ObjectGuid const& guid : candidates)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (helper.IsZombieChow(unit))
            zombies.push_back(unit);
        else if (botAI->EqualLowercaseName(unit->GetName(), "gluth"))
            boss = unit;
    }

    bool const bossTank = botAI->IsMainTank(bot) || botAI->IsAssistTankOfIndex(bot, 0);
    bool const zombieKiter = helper.IsZombieKiter(bot);
    bool const slowdownHunter = helper.IsSlowdownHunter(bot);

    if (bossTank)
    {
        target = boss;
    }
    else if (zombieKiter)
    {
        for (Unit* zombie : zombies)
        {
            if (zombie->GetHealthPct() <= helper.decimatedZombiePct)
                continue;

            bool const loose = !zombie->GetVictim() || zombie->GetVictim() != bot;
            if (!target || (loose && target->GetVictim() == bot) || bot->GetDistance2d(zombie) < bot->GetDistance2d(target))
                target = zombie;
        }

        if (!target)
            target = boss;
    }
    else if (slowdownHunter)
    {
        for (Unit* zombie : zombies)
        {
            if (zombie->GetHealthPct() <= helper.decimatedZombiePct)
                continue;

            bool const headingToBoss = boss && zombie->GetVictim() == boss;
            bool const nearBoss = boss && zombie->GetDistance2d(boss) <= 30.0f;
            if (!headingToBoss && !nearBoss)
                continue;

            if (!target || bot->GetDistance2d(zombie) < bot->GetDistance2d(target))
                target = zombie;
        }

        if (!target)
            target = boss;
    }
    else
    {
        std::pair<float, float> const tankPosition =
            bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL ? helper.mainTankPos25 : helper.mainTankPos10;

        for (Unit* zombie : zombies)
        {
            if (zombie->GetHealthPct() > helper.decimatedZombiePct)
                continue;

            if (!target || zombie->GetDistance2d(tankPosition.first, tankPosition.second) <
                               target->GetDistance2d(tankPosition.first, tankPosition.second))
            {
                target = zombie;
            }
        }

        if (!target)
            target = boss;
    }

    if (!target || AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target, target == boss);
}

bool GluthPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    bool const raid25 = bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
    if (botAI->IsMainTank(bot) || botAI->IsAssistTankOfIndex(bot, 0))
    {
        if (!AI_VALUE2(bool, "has aggro", "boss target"))
            return false;

        std::pair<float, float> const position = raid25 ? helper.mainTankPos25 : helper.mainTankPos10;
        if (MoveTo(NAXX_MAP_ID, position.first, position.second, bot->GetPositionZ(), false, false, false, false,
                   MovementPriority::MOVEMENT_COMBAT))
        {
            return true;
        }

        return MoveInside(NAXX_MAP_ID, position.first, position.second, bot->GetPositionZ(), 2.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }

    if (helper.IsZombieKiter(bot))
    {
        if (helper.BeforeDecimate())
        {
            if (MoveTo(NAXX_MAP_ID, helper.beforeDecimatePos.first, helper.beforeDecimatePos.second, bot->GetPositionZ(), false,
                       false, false, false, MovementPriority::MOVEMENT_COMBAT))
            {
                return true;
            }

            return MoveInside(NAXX_MAP_ID, helper.beforeDecimatePos.first, helper.beforeDecimatePos.second, bot->GetPositionZ(),
                              2.0f, MovementPriority::MOVEMENT_COMBAT);
        }

        Unit* target = AI_VALUE(Unit*, "current target");
        if (helper.IsZombieChow(target) && AI_VALUE2(bool, "has aggro", "current target"))
        {
            uint32 const nearest = FindNearestWaypoint();
            uint32 const nextPoint = (nearest + 1) % intervals;
            return MoveTo(NAXX_MAP_ID, waypoints[nextPoint].first, waypoints[nextPoint].second, bot->GetPositionZ(), false, false,
                          false, false, MovementPriority::MOVEMENT_COMBAT);
        }
    }

    if (botAI->IsRangedDps(bot))
    {
        if (helper.IsSlowdownHunter(bot))
        {
            std::pair<float, float> const slowdownPosition =
                botAI->GetClassIndex(bot, CLASS_HUNTER) == 0 ? helper.leftSlowDownPos : helper.rightSlowDownPos;
            return MoveInside(NAXX_MAP_ID, slowdownPosition.first, slowdownPosition.second, bot->GetPositionZ(), 1.0f,
                              MovementPriority::MOVEMENT_COMBAT);
        }

        return MoveInside(NAXX_MAP_ID, helper.rangedPos.first, helper.rangedPos.second, bot->GetPositionZ(), 3.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }

    if (botAI->IsHeal(bot))
    {
        return MoveInside(NAXX_MAP_ID, helper.healPos.first, helper.healPos.second, bot->GetPositionZ(), 1.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }

    return false;
}

bool GluthSlowdownAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI() || !helper.IsSlowdownHunter(bot) || helper.JustStartCombat())
        return false;

    if (bot->getClass() == CLASS_HUNTER)
        return botAI->CastSpell("frost trap", bot);

    return false;
}
