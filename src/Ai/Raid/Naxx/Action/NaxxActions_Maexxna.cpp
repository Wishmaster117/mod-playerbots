/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <limits>

#include "NaxxSpellIds.h"
#include "Playerbots.h"

bool MaexxnaAttackWebWrapAction::isUseful()
{
    // Ranged DPS break cocoons without dragging melee or healers away from their assignments.
    return !botAI->IsHeal(bot) && !botAI->IsTank(bot) && botAI->IsRanged(bot);
}

bool MaexxnaAttackWebWrapAction::Execute(Event /*event*/)
{
    Unit* bestTarget = nullptr;
    float bestDistance = std::numeric_limits<float>::max();

    GuidVector const targets = AI_VALUE(GuidVector, "possible targets no los");
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->GetEntry() != NaxxSpellIds::MaexxnaWebWrapEntry)
            continue;

        float const distance = bot->GetDistance(unit);
        if (!bestTarget || distance < bestDistance)
        {
            bestTarget = unit;
            bestDistance = distance;
        }
    }

    if (!bestTarget || AI_VALUE(Unit*, "current target") == bestTarget)
        return false;

    return Attack(bestTarget);
}

bool MaexxnaTankSpiderlingsAction::isUseful()
{
    // Keep the main tank on Maexxna. The first available off-tank gathers spiderlings.
    return botAI->IsTank(bot) && !botAI->IsMainTank(bot);
}

bool MaexxnaTankSpiderlingsAction::Execute(Event /*event*/)
{
    Unit* bestTarget = nullptr;
    float bestDistance = std::numeric_limits<float>::max();

    GuidVector const attackers = AI_VALUE(GuidVector, "attackers");
    for (ObjectGuid const& guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->GetEntry() != NaxxSpellIds::MaexxnaSpiderlingEntry)
            continue;

        float const distance = bot->GetDistance(unit);
        if (!bestTarget || distance < bestDistance)
        {
            bestTarget = unit;
            bestDistance = distance;
        }
    }

    if (!bestTarget || AI_VALUE(Unit*, "current target") == bestTarget)
        return false;

    return Attack(bestTarget);
}
