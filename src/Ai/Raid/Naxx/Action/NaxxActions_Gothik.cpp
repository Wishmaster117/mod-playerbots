/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <algorithm>
#include <map>
#include <vector>

#include "NaxxSpellIds.h"
#include "Playerbots.h"

namespace
{
constexpr float GothikLiveX = 2691.2f;
constexpr float GothikLiveY = -3387.0f;
constexpr float GothikLiveZ = 267.68f;
constexpr float GothikDeadX = 2693.5f;
constexpr float GothikDeadY = -3334.6f;
constexpr float GothikDeadZ = 267.68f;

// The inner gate remains closed while Gothik is rooted on the balcony.
bool IsGothikAddPhase(Unit const* boss)
{
    // Before the pull Gothik is already on the balcony (Z > 280). Once engaged,
    // UNIT_FLAG_DISABLE_MOVE remains set until the add phase ends.
    return boss && (boss->GetPositionZ() > 280.0f || boss->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE));
}

enum class GothikSide : uint8
{
    Live,
    Dead
};

bool IsLiveSide(Unit const* unit)
{
    return unit && unit->GetPositionY() < NaxxSpellIds::GothikGateY;
}

bool IsGothikAdd(uint32 entry)
{
    switch (entry)
    {
        case NaxxSpellIds::GothikLivingTraineeEntry:
        case NaxxSpellIds::GothikLivingKnightEntry:
        case NaxxSpellIds::GothikLivingRiderEntry:
        case NaxxSpellIds::GothikDeadTraineeEntry:
        case NaxxSpellIds::GothikDeadKnightEntry:
        case NaxxSpellIds::GothikDeadHorseEntry:
        case NaxxSpellIds::GothikDeadRiderEntry:
            return true;
        default:
            return false;
    }
}

uint32 GetGothikAddPriority(uint32 entry)
{
    switch (entry)
    {
        case NaxxSpellIds::GothikLivingRiderEntry:
            return 70;
        case NaxxSpellIds::GothikLivingKnightEntry:
            return 60;
        case NaxxSpellIds::GothikLivingTraineeEntry:
            return 50;
        case NaxxSpellIds::GothikDeadRiderEntry:
            return 40;
        case NaxxSpellIds::GothikDeadKnightEntry:
            return 30;
        case NaxxSpellIds::GothikDeadHorseEntry:
            return 20;
        case NaxxSpellIds::GothikDeadTraineeEntry:
            return 10;
        default:
            return 0;
    }
}

void CountAssigned(std::map<uint64, GothikSide> const& assignments, uint32& liveCount, uint32& deadCount)
{
    liveCount = 0;
    deadCount = 0;

    for (auto const& assignment : assignments)
    {
        if (assignment.second == GothikSide::Live)
            ++liveCount;
        else
            ++deadCount;
    }
}

void AssignBalanced(std::map<uint64, GothikSide>& assignments, Player* player)
{
    if (!player)
        return;

    uint32 liveCount = 0;
    uint32 deadCount = 0;
    CountAssigned(assignments, liveCount, deadCount);
    assignments[player->GetGUID().GetRawValue()] = liveCount <= deadCount ? GothikSide::Live : GothikSide::Dead;
}

GothikSide GetAssignedGothikSide(PlayerbotAI* botAI, Player* bot)
{
    if (!botAI || !bot)
        return GothikSide::Live;

    Group* group = bot->GetGroup();
    if (!group)
        return GothikSide::Live;

    std::vector<Player*> members;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member->IsAlive() && member->GetMapId() == bot->GetMapId())
            members.push_back(member);
    }

    std::sort(members.begin(), members.end(), [](Player const* left, Player const* right)
    {
        return left->GetGUID().GetRawValue() < right->GetGUID().GetRawValue();
    });

    std::vector<Player*> tanks;
    std::vector<Player*> healers;
    std::vector<Player*> dps;

    for (Player* member : members)
    {
        if (botAI->IsTank(member))
            tanks.push_back(member);
        else if (botAI->IsHeal(member))
            healers.push_back(member);
        else
            dps.push_back(member);
    }

    std::map<uint64, GothikSide> assignments;

    Player* mainTank = nullptr;
    for (Player* tank : tanks)
    {
        if (botAI->IsMainTank(tank))
        {
            mainTank = tank;
            break;
        }
    }
    if (!mainTank && !tanks.empty())
        mainTank = tanks.front();

    if (mainTank)
        assignments[mainTank->GetGUID().GetRawValue()] = GothikSide::Live;

    Player* offTank = nullptr;
    for (Player* tank : tanks)
    {
        if (tank != mainTank && botAI->IsAssistTank(tank))
        {
            offTank = tank;
            break;
        }
    }
    if (!offTank)
    {
        for (Player* tank : tanks)
        {
            if (tank != mainTank)
            {
                offTank = tank;
                break;
            }
        }
    }

    if (offTank)
        assignments[offTank->GetGUID().GetRawValue()] = GothikSide::Dead;

    for (Player* tank : tanks)
    {
        if (tank != mainTank && tank != offTank)
            AssignBalanced(assignments, tank);
    }

    if (!healers.empty())
        assignments[healers[0]->GetGUID().GetRawValue()] = GothikSide::Live;
    if (healers.size() >= 2)
        assignments[healers[1]->GetGUID().GetRawValue()] = GothikSide::Dead;
    for (size_t index = 2; index < healers.size(); ++index)
        AssignBalanced(assignments, healers[index]);

    // DPS are assigned last so the total head count remains as even as possible.
    for (Player* player : dps)
        AssignBalanced(assignments, player);

    auto const found = assignments.find(bot->GetGUID().GetRawValue());
    return found != assignments.end() ? found->second : GothikSide::Live;
}
}  // namespace

bool GothikMoveToAssignedSideAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    if (!boss || bot->GetDistance(boss) > 160.0f || !IsGothikAddPhase(boss))
        return false;

    GothikSide const assignedSide = GetAssignedGothikSide(botAI, bot);
    bool const assignedLive = assignedSide == GothikSide::Live;

    if (assignedLive != IsLiveSide(bot))
        return true;

    float const x = assignedLive ? GothikLiveX : GothikDeadX;
    float const y = assignedLive ? GothikLiveY : GothikDeadY;
    float const z = assignedLive ? GothikLiveZ : GothikDeadZ;
    return bot->GetDistance(x, y, z) > 12.0f;
}

bool GothikMoveToAssignedSideAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    if (!boss || !IsGothikAddPhase(boss))
        return false;

    bool const liveSide = GetAssignedGothikSide(botAI, bot) == GothikSide::Live;
    float const x = liveSide ? GothikLiveX : GothikDeadX;
    float const y = liveSide ? GothikLiveY : GothikDeadY;
    float const z = liveSide ? GothikLiveZ : GothikDeadZ;

    if (MoveTo(NAXX_MAP_ID, x, y, z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
        return true;

    return MoveInside(NAXX_MAP_ID, x, y, z, 3.0f, MovementPriority::MOVEMENT_COMBAT);
}

bool GothikChooseTargetAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    return boss && (bot->IsInCombat() || boss->IsInCombat());
}

bool GothikChooseTargetAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    if (!boss)
        return false;

    bool const botLiveSide = IsLiveSide(bot);
    Unit* bestAdd = nullptr;
    uint32 bestPriority = 0;
    float bestDistance = 0.0f;

    GuidVector const candidates = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid const& guid : candidates)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || !IsGothikAdd(unit->GetEntry()) || IsLiveSide(unit) != botLiveSide)
            continue;

        uint32 const priority = GetGothikAddPriority(unit->GetEntry());
        float const distance = bot->GetDistance(unit);
        if (!bestAdd || priority > bestPriority || (priority == bestPriority && distance < bestDistance))
        {
            bestAdd = unit;
            bestPriority = priority;
            bestDistance = distance;
        }
    }

    Unit* target = bestAdd;

    Creature* bossCreature = boss->ToCreature();
    bool const bossAttackable = bossCreature && bossCreature->IsAlive() &&
                                bossCreature->GetReactState() == REACT_AGGRESSIVE &&
                                !bossCreature->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE);
    if (!target && bossAttackable && (IsLiveSide(boss) == botLiveSide || bot->IsWithinLOSInMap(boss)))
        target = boss;

    if (!target || AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target);
}
