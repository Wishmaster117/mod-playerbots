/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <limits>

#include "NaxxSpellIds.h"

namespace
{
constexpr float MaxFaerlinaAddDistanceToBoss = 60.0f;
}

bool FaerlinaSacrificeWorshipperAction::Execute(Event /*event*/)
{
    Unit* target = FindTarget();
    if (!target || AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target);
}

bool FaerlinaSacrificeWorshipperAction::isUseful()
{
    if (!bot->IsInCombat())
        return false;

    // Reserve the sacrifice/add duty for the first off-tank.
    if (!botAI->IsAssistTankOfIndex(bot, 0))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!boss || !boss->IsAlive())
        return false;

    if (boss->HasAura(NaxxSpellIds::FaerlinaWidowsEmbrace))
        return false;

    if (!boss->HasAura(NaxxSpellIds::FaerlinaFrenzy))
        return false;

    return FindTarget() != nullptr;
}

Unit* FaerlinaSacrificeWorshipperAction::FindTarget()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!boss)
        return nullptr;

    Creature* bestWorshipper = nullptr;
    float bestWorshipperDistance = std::numeric_limits<float>::max();

    Creature* bestFollower = nullptr;
    float bestFollowerDistance = std::numeric_limits<float>::max();

    GuidVector const npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (ObjectGuid const& guid : npcs)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        Creature* creature = unit->ToCreature();
        if (!creature || !creature->IsWithinDistInMap(boss, MaxFaerlinaAddDistanceToBoss))
            continue;

        float const distanceToBoss = creature->GetDistance(boss);
        switch (creature->GetEntry())
        {
            case NaxxSpellIds::NaxxramasWorshipperEntry:
                if (distanceToBoss < bestWorshipperDistance)
                {
                    bestWorshipper = creature;
                    bestWorshipperDistance = distanceToBoss;
                }
                break;
            case NaxxSpellIds::NaxxramasFollowerEntry:
                if (distanceToBoss < bestFollowerDistance)
                {
                    bestFollower = creature;
                    bestFollowerDistance = distanceToBoss;
                }
                break;
            default:
                break;
        }
    }

    // Worshippers perform the intended Widow's Embrace mechanic. Followers are only a clean-up fallback.
    return bestWorshipper ? static_cast<Unit*>(bestWorshipper) : static_cast<Unit*>(bestFollower);
}
