#include "RaidNaxxActions.h"

#include <cfloat>

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "SharedDefines.h"

bool GluthChooseTargetAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }
    GuidVector attackers = context->GetValue<GuidVector>("possible targets")->Get();
    Unit* target = nullptr;
    Unit* target_boss = nullptr;
    std::vector<Unit*> target_zombies;
    for (GuidVector::iterator i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit)
            continue;
        if (!unit->IsAlive())
        {
            continue;
        }
        if (botAI->EqualLowercaseName(unit->GetName(), "zombie chow"))
        {
            target_zombies.push_back(unit);
        }
        if (botAI->EqualLowercaseName(unit->GetName(), "gluth"))
        {
            target_boss = unit;
        }
    }
    if (botAI->IsMainTank(bot) || botAI->IsAssistTankOfIndex(bot, 0))
    {
        target = target_boss;
    }
    else if (botAI->IsAssistTankOfIndex(bot, 1))
    {
        for (Unit* t : target_zombies)
        {
            if (helper.IsZombieHealthy(t) && t->GetVictim() != bot && t->GetDistance2d(bot) <= 10.0f)
            {
                if (!target || t->GetDistance2d(bot) < target->GetDistance2d(bot))
                {
                    target = t;
                }
            }
        }
    }
    else if (botAI->GetClassIndex(bot, CLASS_HUNTER) == 0 || botAI->GetClassIndex(bot, CLASS_HUNTER) == 1)
    {
        // prevent zombie go straight to gluth
        for (Unit* t : target_zombies)
        {
            if (helper.IsZombieHealthy(t) && t->GetVictim() == target_boss &&
                t->GetDistance2d(bot) <= sPlayerbotAIConfig->spellDistance)
            {
                if (!target || t->GetDistance2d(bot) < target->GetDistance2d(bot))
                {
                    target = t;
                }
            }
        }
        if (!target)
        {
            target = target_boss;
        }
    }
    else
    {
        bool raid25 = bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
        float tankPosX = raid25 ? helper.mainTankPos25.first : helper.mainTankPos10.first;
        float tankPosY = raid25 ? helper.mainTankPos25.second : helper.mainTankPos10.second;

        for (Unit* t : target_zombies)
        {
            if (helper.IsZombieDecimated(t))
            {
                if (target == nullptr ||
                    target->GetDistance2d(tankPosX, tankPosY) > t->GetDistance2d(tankPosX, tankPosY))
                {
                    target = t;
                }
            }
        }
        if (target == nullptr)
        {
            target = target_boss;
        }
    }
    if (!target || context->GetValue<Unit*>("current target")->Get() == target)
    {
        return false;
    }
    if (target_boss && target == target_boss)
        return Attack(target, true);
    return Attack(target, false);
    // return Attack(target);
}

bool GluthPositionAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }
    bool raid25 = bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
    if (botAI->IsMainTank(bot) || botAI->IsAssistTankOfIndex(bot, 0))
    {
        if (AI_VALUE2(bool, "has aggro", "boss target"))
        {
            if (raid25)
            {
                if (MoveTo(NAXX_MAP_ID, helper.mainTankPos25.first, helper.mainTankPos25.second, bot->GetPositionZ(), false, false, false,
                           false, MovementPriority::MOVEMENT_COMBAT))
                {
                    return true;
                }
                return MoveInside(NAXX_MAP_ID, helper.mainTankPos25.first, helper.mainTankPos25.second, bot->GetPositionZ(), 2.0f,
                                  MovementPriority::MOVEMENT_COMBAT);
            }
            else
            {
                if (MoveTo(NAXX_MAP_ID, helper.mainTankPos10.first, helper.mainTankPos10.second, bot->GetPositionZ(), false, false, false,
                           false, MovementPriority::MOVEMENT_COMBAT))
                {
                    return true;
                }
                return MoveInside(NAXX_MAP_ID, helper.mainTankPos10.first, helper.mainTankPos10.second, bot->GetPositionZ(), 2.0f,
                                  MovementPriority::MOVEMENT_COMBAT);
            }
        }
    }
    else if (botAI->IsAssistTankOfIndex(bot, 1))
    {
        if (helper.BeforeDecimate())
        {
            if (MoveTo(bot->GetMapId(), helper.beforeDecimatePos.first, helper.beforeDecimatePos.second, bot->GetPositionZ(), false, false,
                       false, false, MovementPriority::MOVEMENT_COMBAT))
            {
                return true;
            }
            return MoveInside(bot->GetMapId(), helper.beforeDecimatePos.first, helper.beforeDecimatePos.second, bot->GetPositionZ(), 2.0f,
                              MovementPriority::MOVEMENT_COMBAT);
        }
        else
        {
            if (AI_VALUE2(bool, "has aggro", "current target"))
            {
                uint32 nearest = FindNearestWaypoint();
                uint32 next_point = (nearest + 1) % intervals;
                return MoveTo(bot->GetMapId(), waypoints[next_point].first, waypoints[next_point].second, bot->GetPositionZ(),
                              false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
            }
        }
    }
    else if (botAI->IsRangedDps(bot))
    {
        if (raid25)
        {
            if (botAI->GetClassIndex(bot, CLASS_HUNTER) == 0)
            {
                return MoveInside(NAXX_MAP_ID, helper.leftSlowDownPos.first, helper.leftSlowDownPos.second, bot->GetPositionZ(), 0.0f,
                                  MovementPriority::MOVEMENT_COMBAT);
            }
            if (botAI->GetClassIndex(bot, CLASS_HUNTER) == 1)
            {
                return MoveInside(NAXX_MAP_ID, helper.rightSlowDownPos.first, helper.rightSlowDownPos.second, bot->GetPositionZ(), 0.0f,
                                  MovementPriority::MOVEMENT_COMBAT);
            }
        }
        return MoveInside(NAXX_MAP_ID, helper.rangedPos.first, helper.rangedPos.second, bot->GetPositionZ(), 3.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }
    else if (botAI->IsHeal(bot))
    {
        return MoveInside(NAXX_MAP_ID, helper.healPos.first, helper.healPos.second, bot->GetPositionZ(), 0.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool GluthSlowdownAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }
    if (helper.JustStartCombat())
    {
        return false;
    }
    GuidVector attackers = context->GetValue<GuidVector>("possible targets")->Get();
    Unit* nearestZombie = nullptr;
    float nearestDistance = FLT_MAX;

    for (ObjectGuid const& guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
        {
            continue;
        }
        if (!helper.IsZombieChow(unit))
        {
            continue;
        }
        float distance = bot->GetDistance2d(unit);
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestZombie = unit;
        }
    }
    if (!nearestZombie)
    {
        return false;
    }

    switch (bot->getClass())
    {
        case CLASS_HUNTER:
            return botAI->CastSpell("frost trap", bot);
            break;
        case CLASS_MAGE:
            if (nearestDistance <= 12.0f)
            {
                return botAI->CastSpell("frost nova", bot);
            }
            break;
        case CLASS_SHAMAN:
            return botAI->CastSpell("earthbind totem", bot);
            break;
        case CLASS_PALADIN:
            if (nearestDistance <= 12.0f)
            {
                if (botAI->CastSpell("consecration", bot))
                {
                    return true;
                }
                return botAI->CastSpell("holy wrath", nearestZombie);
            }
            break;
        default:
            break;
    }
    return false;
}