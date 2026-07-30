/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include "NaxxSpellIds.h"
#include "ObjectGuid.h"
#include "Playerbots.h"

bool AnubrekhanChooseTargetAction::Execute(Event /*event*/)
{
    GuidVector const attackers = AI_VALUE(GuidVector, "attackers");

    Unit* boss = nullptr;
    std::vector<Unit*> cryptGuards;
    std::vector<Unit*> corpseScarabs;

    for (ObjectGuid const& guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (botAI->EqualLowercaseName(unit->GetName(), "anub'rekhan"))
            boss = unit;
        else if (botAI->EqualLowercaseName(unit->GetName(), "crypt guard"))
            cryptGuards.push_back(unit);
        else if (botAI->EqualLowercaseName(unit->GetName(), "corpse scarab"))
            corpseScarabs.push_back(unit);
    }

    Unit* target = nullptr;

    if (botAI->IsMainTank(bot))
    {
        target = boss;
    }
    else if (botAI->IsAssistTank(bot))
    {
        // Pick up a Crypt Guard that is not already controlled by another tank.
        for (Unit* guard : cryptGuards)
        {
            Player* victim = guard->GetVictim() ? guard->GetVictim()->ToPlayer() : nullptr;
            if (!victim || !botAI->IsTank(victim))
            {
                target = guard;
                break;
            }
        }

        if (!target && !cryptGuards.empty())
            target = cryptGuards.front();

        if (!target)
            target = boss;
    }
    else if (!cryptGuards.empty())
    {
        // Finish the weakest guard first to reduce incoming raid damage.
        for (Unit* guard : cryptGuards)
        {
            if (!target || guard->GetHealthPct() < target->GetHealthPct())
                target = guard;
        }
    }
    else if (!corpseScarabs.empty())
    {
        // Prefer scarabs attacking a non-tank; otherwise take the closest available one.
        for (Unit* scarab : corpseScarabs)
        {
            Player* victim = scarab->GetVictim() ? scarab->GetVictim()->ToPlayer() : nullptr;
            if (victim && !botAI->IsTank(victim))
            {
                target = scarab;
                break;
            }
        }

        if (!target)
        {
            for (Unit* scarab : corpseScarabs)
            {
                if (!target || bot->GetDistance(scarab) < bot->GetDistance(target))
                    target = scarab;
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

bool AnubrekhanPositionAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
        return false;

    bool const locustSwarm =
        NaxxSpellIds::HasAnyAura(
            boss, {NaxxSpellIds::LocustSwarm10, NaxxSpellIds::LocustSwarm10Alt, NaxxSpellIds::LocustSwarm25}) ||
        botAI->HasAura("locust swarm", boss);

    if (!locustSwarm)
        return false;

    if (botAI->IsMainTank(bot))
    {
        uint32 const nearest = FindNearestWaypoint();
        uint32 const nextPoint = (nearest + 1) % intervals;
        return MoveTo(bot->GetMapId(), waypoints[nextPoint].first, waypoints[nextPoint].second, bot->GetPositionZ(),
                      false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }

    return MoveInside(NAXX_MAP_ID, 3272.49f, -3476.27f, bot->GetPositionZ(), 3.0f,
                      MovementPriority::MOVEMENT_COMBAT);
}
