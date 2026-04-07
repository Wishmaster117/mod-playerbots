#include "RaidNaxxActions.h"

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "RaidNaxxBossHelper.h"
#include "RaidNaxxSpellIds.h"

bool NothChooseTargetAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }

    GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
    Unit* target = nullptr;
    Unit* target_boss = nullptr;
    Unit* target_champion = nullptr;
    Unit* target_guardian = nullptr;
    Unit* target_warrior = nullptr;
    Unit* target_assist_add = nullptr;
    Unit* target_main_add = nullptr;
    Unit* target_unassigned_add = nullptr;
    Player* assist_tank = helper.GetAliveAssistTank();
    bool has_assist_tank = assist_tank != nullptr;

    for (auto i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit || !unit->IsAlive())
        {
            continue;
        }

        if (botAI->EqualLowercaseName(unit->GetName(), "noth the plaguebringer"))
        {
            target_boss = unit;
        }
        else if (botAI->EqualLowercaseName(unit->GetName(), "plagued champion"))
        {
            if (!target_champion || bot->GetDistance2d(unit) < bot->GetDistance2d(target_champion))
            {
                target_champion = unit;
            }
            Player* victim = unit->GetVictim() ? unit->GetVictim()->ToPlayer() : nullptr;
            if (victim && botAI->IsAssistTank(victim))
            {
                if (!target_assist_add || bot->GetDistance2d(unit) < bot->GetDistance2d(target_assist_add))
                {
                    target_assist_add = unit;
                }
            }
            else if (victim && botAI->IsMainTank(victim))
            {
                if (!target_main_add || bot->GetDistance2d(unit) < bot->GetDistance2d(target_main_add))
                {
                    target_main_add = unit;
                }
            }
            else if (!target_unassigned_add || bot->GetDistance2d(unit) < bot->GetDistance2d(target_unassigned_add))
            {
                target_unassigned_add = unit;
            }
        }
        else if (botAI->EqualLowercaseName(unit->GetName(), "plagued guardian"))
        {
            if (!target_guardian || bot->GetDistance2d(unit) < bot->GetDistance2d(target_guardian))
            {
                target_guardian = unit;
            }
            Player* victim = unit->GetVictim() ? unit->GetVictim()->ToPlayer() : nullptr;
            if (victim && botAI->IsAssistTank(victim))
            {
                if (!target_assist_add || bot->GetDistance2d(unit) < bot->GetDistance2d(target_assist_add))
                {
                    target_assist_add = unit;
                }
            }
            else if (victim && botAI->IsMainTank(victim))
            {
                if (!target_main_add || bot->GetDistance2d(unit) < bot->GetDistance2d(target_main_add))
                {
                    target_main_add = unit;
                }
            }
            else if (!target_unassigned_add || bot->GetDistance2d(unit) < bot->GetDistance2d(target_unassigned_add))
            {
                target_unassigned_add = unit;
            }
        }
        else if (botAI->EqualLowercaseName(unit->GetName(), "plagued warrior"))
        {
            if (!target_warrior || bot->GetDistance2d(unit) < bot->GetDistance2d(target_warrior))
            {
                target_warrior = unit;
            }
        }
    }

    std::vector<Unit*> targets;
    bool is_main_tank = botAI->IsMainTank(bot);
    bool is_assist_tank = botAI->IsAssistTank(bot);
    bool should_handle_adds = is_assist_tank || (is_main_tank && !has_assist_tank);

    if (should_handle_adds)
    {
        Unit* warrior_needs_pickup = nullptr;
        for (auto i = attackers.begin(); i != attackers.end(); ++i)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (!unit || !unit->IsAlive())
            {
                continue;
            }
            if (!botAI->EqualLowercaseName(unit->GetName(), "plagued warrior"))
            {
                continue;
            }
            if (unit->GetVictim() && unit->GetVictim()->ToPlayer() &&
                ((is_assist_tank && !botAI->IsAssistTank(unit->GetVictim()->ToPlayer())) ||
                 (is_main_tank && !has_assist_tank && !botAI->IsMainTank(unit->GetVictim()->ToPlayer()))))
            {
                warrior_needs_pickup = unit;
                break;
            }
        }
        if (helper.IsBalconyPhase())
        {
            Unit* assigned_add = is_assist_tank ? target_assist_add : target_main_add;
            targets = {assigned_add, target_unassigned_add, target_champion, target_guardian, target_warrior};
        }
        else
        {
            if (is_assist_tank)
            {
                targets = {warrior_needs_pickup, target_warrior};
            }
            else
            {
                targets = {warrior_needs_pickup, target_warrior, target_boss};
            }
        }
    }
    else if (helper.IsBalconyPhase())
    {
        if (has_assist_tank)
        {
            targets = {target_assist_add, target_main_add, target_unassigned_add, target_champion, target_guardian, target_warrior};
        }
        else
        {
            targets = {target_champion, target_guardian, target_unassigned_add, target_warrior};
        }
    }
    else
    {
        targets = {target_boss};
    }

    for (Unit* t : targets)
    {
        if (t)
        {
            target = t;
            break;
        }
    }

    if (!target || context->GetValue<Unit*>("current target")->Get() == target)
    {
        return false;
    }

    return Attack(target);
}

bool NothPositionAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }

    if (botAI->IsAssistTank(bot))
    {
        GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
        Unit* loose_warrior = nullptr;
        for (auto i = attackers.begin(); i != attackers.end(); ++i)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (!unit || !unit->IsAlive())
            {
                continue;
            }
            if (!botAI->EqualLowercaseName(unit->GetName(), "plagued warrior"))
            {
                continue;
            }
            if (unit->GetVictim() && unit->GetVictim()->ToPlayer() &&
                !botAI->IsAssistTank(unit->GetVictim()->ToPlayer()))
            {
                loose_warrior = unit;
                break;
            }
        }
        if (loose_warrior && bot->GetDistance2d(loose_warrior) > 5.0f)
        {
            return MoveTo(NAXX_MAP_ID, loose_warrior->GetPositionX(), loose_warrior->GetPositionY(),
                          loose_warrior->GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }
        Unit* currentTarget = AI_VALUE(Unit*, "current target");
        if (currentTarget && AI_VALUE2(bool, "has aggro", "current target"))
        {
            bool isNothAdd = botAI->EqualLowercaseName(currentTarget->GetName(), "plagued warrior") ||
                             botAI->EqualLowercaseName(currentTarget->GetName(), "plagued champion") ||
                             botAI->EqualLowercaseName(currentTarget->GetName(), "plagued guardian");
            if (isNothAdd)
            {
                GuidVector members = AI_VALUE(GuidVector, "group members");
                Unit* closestHealer = nullptr;
                float closestHealerDistance = 0.0f;
                for (ObjectGuid const& guid : members)
                {
                    Unit* member = botAI->GetUnit(guid);
                    if (!member || member == bot || !member->IsAlive())
                    {
                        continue;
                    }

                    Player* memberPlayer = member->ToPlayer();
                    if (!memberPlayer || !botAI->IsHeal(memberPlayer))
                    {
                        continue;
                    }

                    float distance = bot->GetDistance2d(member);
                    if (!closestHealer || distance < closestHealerDistance)
                    {
                        closestHealer = member;
                        closestHealerDistance = distance;
                    }
                }

                if (closestHealer && closestHealerDistance > 30.0f)
                {
                    float angle = closestHealer->GetAngle(bot);
                    float dx = closestHealer->GetPositionX() + cos(angle) * 25.0f;
                    float dy = closestHealer->GetPositionY() + sin(angle) * 25.0f;
                    return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false,
                                  MovementPriority::MOVEMENT_COMBAT);
                }
            }
        }
        if (currentTarget && botAI->EqualLowercaseName(currentTarget->GetName(), "plagued warrior"))
        {
            GuidVector friendlyPlayers = AI_VALUE(GuidVector, "nearest friendly players");
            Unit* closestPlayer = nullptr;
            float closestDistance = 0.0f;
            for (ObjectGuid const& guid : friendlyPlayers)
            {
                Unit* member = botAI->GetUnit(guid);
                if (!member || member == bot)
                {
                    continue;
                }
                float distance = bot->GetDistance2d(member);
                if (distance <= 5.0f && (!closestPlayer || distance < closestDistance))
                {
                    closestPlayer = member;
                    closestDistance = distance;
                }
            }
            if (closestPlayer)
            {
                float angle = closestPlayer->GetAngle(bot);
                float dx = closestPlayer->GetPositionX() + cos(angle) * 5.0f;
                float dy = closestPlayer->GetPositionY() + sin(angle) * 5.0f;
                return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false,
                              MovementPriority::MOVEMENT_COMBAT);
            }
        }
        return false;
    }

    if (!helper.IsBalconyPhase() || !botAI->IsRanged(bot))
    {
        return false;
    }

    GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
    Unit* nearest_champion = nullptr;
    float nearest_distance = 0.0f;

    for (auto i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit || !unit->IsAlive())
        {
            continue;
        }
        if (!botAI->EqualLowercaseName(unit->GetName(), "plagued champion"))
        {
            continue;
        }
        float distance = bot->GetDistance2d(unit);
        if (!nearest_champion || distance < nearest_distance)
        {
            nearest_champion = unit;
            nearest_distance = distance;
        }
    }

    if (nearest_champion && nearest_distance < 25.0f)
    {
        float angle = nearest_champion->GetAngle(bot);
        float dx = nearest_champion->GetPositionX() + cos(angle) * 25.0f;
        float dy = nearest_champion->GetPositionY() + sin(angle) * 25.0f;
        return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }

    return false;
}