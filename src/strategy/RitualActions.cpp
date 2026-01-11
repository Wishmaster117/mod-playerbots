/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "RitualActions.h"

#include "Battleground.h"
#include "GameObject.h"
#include "GenericBuffUtils.h"
#include "Item.h"
#include "Group.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include <unordered_map>
#include <sstream>
#include <string>
#include <ctime>

namespace
{
    constexpr uint32 kConjuredFoodTargetCount = 20;
    constexpr uint32 kRitualDelayMs = 2000;
    constexpr float kGroupMemberDistance = RITUAL_GROUP_CHECK_RANGE;
    constexpr uint32 kRitualLogCooldownSeconds = 10;
    constexpr uint32 kConjuredFoodIds[] = {
        43523, // Conjured Mana Biscuit
        43518, // Conjured Mana Strudel
        43517, // Conjured Mana Cookie
        43516, // Conjured Mana Cake
        43515, // Conjured Mana Pie
        43514, // Conjured Mana Bread
        43513, // Conjured Mana Muffin
        43512, // Conjured Mana Donut
        43511, // Conjured Mana Bagel
        43510  // Conjured Mana Pretzel
    };
    constexpr uint32 kConjuredWaterIds[] = {
        43519, // Conjured Mana Water
        43520, // Conjured Mana Juice
        43521, // Conjured Mana Tea
        43522  // Conjured Mana Coffee
    };

    uint32 GetRitualOfRefreshmentSpellId(Player* bot)
    {
        if (bot->HasSpell(RITUAL_OF_REFRESHMENT_RANK_2))
            return RITUAL_OF_REFRESHMENT_RANK_2;
        if (bot->HasSpell(RITUAL_OF_REFRESHMENT_RANK_1))
            return RITUAL_OF_REFRESHMENT_RANK_1;
        return 0;
    }

    uint32 GetRitualOfSoulsSpellId(Player* bot)
    {
        if (bot->HasSpell(RITUAL_OF_SOULS_RANK_2))
            return RITUAL_OF_SOULS_RANK_2;
        if (bot->HasSpell(RITUAL_OF_SOULS_RANK_1))
            return RITUAL_OF_SOULS_RANK_1;
        return 0;
    }

    uint32 CountItems(std::vector<Item*> const& items)
    {
        uint32 count = 0;
        for (Item* item : items)
        {
            if (item)
                count += item->GetCount();
        }
        return count;
    }

    template <size_t N>
    bool HasAnyItem(Player const* player, uint32 const (&entries)[N])
    {
        for (uint32 entry : entries)
        {
            if (player->GetItemCount(entry, false) > 0)
                return true;
        }

        return false;
    }

    bool GroupNeedsRefreshment(Player* bot)
    {
        Group* group = bot->GetGroup();
        if (!group)
            return false;

        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || !member->IsAlive())
                continue;

            if (member->GetMap() != bot->GetMap())
                continue;

            if (bot->GetDistance(member) > kGroupMemberDistance)
                continue;

            if (!HasAnyItem(member, kConjuredFoodIds) || !HasAnyItem(member, kConjuredWaterIds))
                return true;
        }

        return false;
    }

    bool HasNearbySoulwell(Player* bot)
    {
        return FindNearestRitualObject(
            bot,
            {RITUAL_SOUL_WELL_RANK_1, RITUAL_SOUL_WELL_RANK_2, RITUAL_SOUL_WELL_RANK_2_VARIANT_1, RITUAL_SOUL_WELL_RANK_2_VARIANT_2},
            GetRitualSearchRange(bot));
    }

    bool HasNearbySoulPortal(Player* bot)
    {
        return FindNearestRitualObject(bot, {RITUAL_SOUL_PORTAL_RANK_1, RITUAL_SOUL_PORTAL_RANK_2}, GetRitualSearchRange(bot));
    }

    bool HasNearbyRefreshmentPortal(Player* bot)
    {
        return FindNearestRitualObject(bot, {RITUAL_REFRESHMENT_PORTAL_RANK_1, RITUAL_REFRESHMENT_PORTAL_RANK_2}, GetRitualSearchRange(bot));
    }

    bool HasNearbyRefreshmentTable(Player* bot)
    {
        return FindNearestRitualObject(bot, {RITUAL_REFRESHMENT_TABLE_RANK_1, RITUAL_REFRESHMENT_TABLE_RANK_2}, GetRitualSearchRange(bot));
    }

    bool GroupNeedsHealthstone(Player* bot)
    {
        Group* group = bot->GetGroup();
        if (!group)
            return false;

        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || !member->IsAlive())
                continue;

            if (member->GetMap() != bot->GetMap())
                continue;

            if (bot->GetDistance(member) > kGroupMemberDistance)
                continue;

            if (!HasHealthstone(member))
                return true;
        }

        return false;
    }

    std::unordered_map<ObjectGuid, uint32> lastRefreshmentLog;
    std::unordered_map<ObjectGuid, uint32> lastSoulsLog;

    bool ShouldLogForBot(Player* bot, std::unordered_map<ObjectGuid, uint32>& logTimes)
    {
        uint32 now = time(nullptr);
        auto it = logTimes.find(bot->GetGUID());
        if (it != logTimes.end() && (now - it->second) < kRitualLogCooldownSeconds)
            return false;

        logTimes[bot->GetGUID()] = now;
        return true;
    }

    void LogRefreshmentDecision(Player* bot, char const* reason)
    {
        if (!bot || !ShouldLogForBot(bot, lastRefreshmentLog))
            return;

        LOG_INFO("playerbots", "Ritual of Refreshment skipped for bot {} (GUID: {}): {}", bot->GetName(), bot->GetGUID().GetCounter(), reason);
    }

    void LogSoulsDecision(Player* bot, char const* reason)
    {
        if (!bot || !ShouldLogForBot(bot, lastSoulsLog))
            return;

        LOG_INFO("playerbots", "Ritual of Souls skipped for bot {} (GUID: {}): {}", bot->GetName(), bot->GetGUID().GetCounter(), reason);
    }
}

float GetRitualSearchRange(Player* bot)
{
    return bot->GetMap() && bot->GetMap()->IsBattleground() ? RITUAL_BATTLEGROUND_SEARCH_RANGE : RITUAL_DUNGEON_SEARCH_RANGE;
}

bool IsRitualMap(Player* bot)
{
    if (!bot || !bot->GetMap())
        return false;

    return bot->GetMap()->IsDungeon() || bot->GetMap()->IsRaid() || bot->GetMap()->IsBattleground();
}

bool CanUseRituals(Player* bot, std::string* reason)
{
    if (!bot || !bot->GetMap())
    {
        if (reason)
            *reason = "missing map";
        return false;
    }

    if (!IsRitualMap(bot))
    {
        if (reason)
        {
            Map* map = bot->GetMap();
            std::ostringstream details;
            details << "map not dungeon/raid/bg (mapId: " << map->GetId()
                    << ", instance: " << (map->Instanceable() ? "yes" : "no")
                    << ", dungeon: " << (map->IsDungeon() ? "yes" : "no")
                    << ", raid: " << (map->IsRaid() ? "yes" : "no")
                    << ", bg: " << (map->IsBattleground() ? "yes" : "no")
                    << ")";
            *reason = details.str();
        }
        return false;
    }

    if (bot->GetMap()->IsBattleground())
    {
        Battleground* bg = bot->GetBattleground();
        if (!bg)
        {
            if (reason)
                *reason = "battleground not ready";
            return false;
        }

        if (bg->GetStatus() != STATUS_WAIT_JOIN)
        {
            if (reason)
            {
                std::ostringstream details;
                details << "battleground status " << bg->GetStatus();
                *reason = details.str();
            }
            return false;
        }
    }

    return true;
}

bool CanUseRituals(Player* bot)
{
    return CanUseRituals(bot, nullptr);
}

GameObject* FindNearestRitualObject(Player* bot, std::initializer_list<uint32> entries, float range)
{
    GameObject* nearest = nullptr;
    float bestDistance = 0.0f;

    for (uint32 entry : entries)
    {
        GameObject* candidate = bot->FindNearestGameObject(entry, range);
        if (!candidate)
            continue;

        float distance = bot->GetDistance(candidate);
        if (!nearest || distance < bestDistance)
        {
            nearest = candidate;
            bestDistance = distance;
        }
    }

    return nearest;
}

bool HasHealthstone(Player const* player)
{
    if (!player)
        return false;

    const uint32 healthstoneIds[] = {
        RITUAL_MINOR_HEALTHSTONE,
        RITUAL_LESSER_HEALTHSTONE,
        RITUAL_MAJOR_HEALTHSTONE,
        RITUAL_MINOR_HEALTHSTONE_ALT,
        RITUAL_LESSER_HEALTHSTONE_ALT,
        RITUAL_FEL_HEALTHSTONE,
        RITUAL_DEMONIC_HEALTHSTONE};

    for (uint32 id : healthstoneIds)
    {
        if (player->GetItemCount(id, false) > 0)
            return true;
    }

    return false;
}

bool BotNeedsHealthstone(PlayerbotAI* botAI)
{
    if (!botAI)
        return false;

    AiObjectContext* ctx = botAI->GetAiObjectContext();
    if (!ctx)
        return false;

    return ctx->GetValue<uint32>("item count", "healthstone")->Get() == 0;
}

bool BotNeedsConjuredFoodOrWater(PlayerbotAI* botAI, uint32 desiredCount)
{
    if (!botAI)
        return false;

    AiObjectContext* ctx = botAI->GetAiObjectContext();
    if (!ctx)
        return false;

    uint32 foodCount = CountItems(ctx->GetValue<std::vector<Item*>>("inventory items", "conjured food")->Get());
    uint32 waterCount = CountItems(ctx->GetValue<std::vector<Item*>>("inventory items", "conjured water")->Get());
    return foodCount < desiredCount || waterCount < desiredCount;
}

bool IsPrimaryRitualCaster(Player* bot, uint8 classId)
{
    Group* group = bot->GetGroup();
    if (!group)
        return true;

    ObjectGuid bestGuid;
    bool found = false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member->getClass() != classId || !member->IsAlive())
            continue;

        if (bot->GetMap()->IsBattleground() && member->GetTeamId() != bot->GetTeamId())
            continue;

        if (!found || member->GetGUID().GetCounter() < bestGuid.GetCounter())
        {
            bestGuid = member->GetGUID();
            found = true;
        }
    }

    return !found || bestGuid == bot->GetGUID();
}

bool ShouldCastRitualOfRefreshment(PlayerbotAI* botAI)
{
    Player* bot = botAI->GetBot();
    if (!bot || bot->getClass() != CLASS_MAGE)
    {
        LogRefreshmentDecision(bot, "not a mage");
        return false;
    }

    std::string ritualReason;
    if (!CanUseRituals(bot, &ritualReason))
    {
        LogRefreshmentDecision(bot, ritualReason.empty() ? "rituals not allowed in this map/state" : ritualReason.c_str());
        return false;
    }

    uint32 spellId = GetRitualOfRefreshmentSpellId(bot);
    if (!spellId)
    {
        LogRefreshmentDecision(bot, "spell not known");
        return false;
    }

    if (bot->GetSpellCooldownDelay(spellId) > 0)
    {
        LogRefreshmentDecision(bot, "spell on cooldown");
        return false;
    }


    if (!ai::buff::HasRequiredReagents(bot, spellId))
    {
        LogRefreshmentDecision(bot, "missing reagents");
        return false;
    }

    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
    {
        LogRefreshmentDecision(bot, "currently channeling");
        return false;
    }

    Group* group = bot->GetGroup();
    if (!group || group->GetMembersCount() < 2)
    {
        LogRefreshmentDecision(bot, "not in group or group too small");
        return false;
    }

    if (!IsPrimaryRitualCaster(bot, CLASS_MAGE))
    {
        LogRefreshmentDecision(bot, "not primary caster");
        return false;
    }

    if (HasNearbyRefreshmentTable(bot))
    {
        LogRefreshmentDecision(bot, "refreshment table already nearby");
        return false;
    }

    if (!GroupNeedsRefreshment(bot))
    {
        LogRefreshmentDecision(bot, "group already has conjured food/water");
        return false;
    }

    return true;
}

bool ShouldCastRitualOfSouls(PlayerbotAI* botAI)
{
    Player* bot = botAI->GetBot();
    if (!bot || bot->getClass() != CLASS_WARLOCK)
    {
        LogSoulsDecision(bot, "not a warlock");
        return false;
    }

    std::string ritualReason;
    if (!CanUseRituals(bot, &ritualReason))
    {
        LogSoulsDecision(bot, ritualReason.empty() ? "rituals not allowed in this map/state" : ritualReason.c_str());
        return false;
    }

    uint32 spellId = GetRitualOfSoulsSpellId(bot);
    if (!spellId)
    {
        LogSoulsDecision(bot, "spell not known");
        return false;
    }

    if (bot->GetSpellCooldownDelay(spellId) > 0)
    {
        LogSoulsDecision(bot, "spell on cooldown");
        return false;
    }

    if (!ai::buff::HasRequiredReagents(bot, spellId))
    {
        LogSoulsDecision(bot, "missing reagents");
        return false;
    }

    Group* group = bot->GetGroup();
    if (!group || group->GetMembersCount() < 2)
    {
        LogSoulsDecision(bot, "not in group or group too small");
        return false;
    }

    if (!IsPrimaryRitualCaster(bot, CLASS_WARLOCK))
    {
        LogSoulsDecision(bot, "not primary caster");
        return false;
    }

    if (HasNearbySoulPortal(bot) || HasNearbySoulwell(bot))
    {
        LogSoulsDecision(bot, "soul portal or soulwell already nearby");
        return false;
    }

    if (!GroupNeedsHealthstone(bot))
    {
        LogSoulsDecision(bot, "group already has healthstones");
        return false;
    }

    return true;
}

bool InteractWithSoulPortalAction::isUseful()
{
    if (!CanUseRituals(bot))
        return false;

    if (!BotNeedsHealthstone(botAI))
        return false;

    return HasNearbySoulPortal(bot);
}

bool InteractWithSoulPortalAction::Execute(Event event)
{
    GameObject* soulPortal = FindNearestRitualObject(
        bot,
        {RITUAL_SOUL_PORTAL_RANK_1, RITUAL_SOUL_PORTAL_RANK_2},
        GetRitualSearchRange(bot));
    if (!soulPortal)
        return false;

    if (bot->GetDistance(soulPortal) > RITUAL_INTERACTION_DISTANCE)
    {
        return MoveTo(bot->GetMapId(), soulPortal->GetPositionX(), soulPortal->GetPositionY(), soulPortal->GetPositionZ(), false, false);
    }

    soulPortal->Use(bot);
    botAI->SetNextCheckDelay(kRitualDelayMs);
    return true;
}

bool InteractWithSoulwellAction::isUseful()
{
    if (!CanUseRituals(bot))
        return false;

    if (bot->getClass() == CLASS_WARLOCK)
        return false;

    if (!BotNeedsHealthstone(botAI))
        return false;

    return HasNearbySoulwell(bot);
}

bool InteractWithSoulwellAction::Execute(Event event)
{
    GameObject* soulwell = FindNearestRitualObject(
        bot,
        {RITUAL_SOUL_WELL_RANK_1, RITUAL_SOUL_WELL_RANK_2, RITUAL_SOUL_WELL_RANK_2_VARIANT_1, RITUAL_SOUL_WELL_RANK_2_VARIANT_2},
        GetRitualSearchRange(bot));
    if (!soulwell)
        return false;

    if (bot->GetDistance(soulwell) > RITUAL_INTERACTION_DISTANCE)
    {
        return MoveTo(bot->GetMapId(), soulwell->GetPositionX(), soulwell->GetPositionY(), soulwell->GetPositionZ(), false, false);
    }

    soulwell->Use(bot);
    botAI->SetNextCheckDelay(kRitualDelayMs);
    return true;
}

bool InteractWithRefreshmentPortalAction::isUseful()
{
    if (!CanUseRituals(bot))
        return false;

    if (!BotNeedsConjuredFoodOrWater(botAI, kConjuredFoodTargetCount))
        return false;

    return HasNearbyRefreshmentPortal(bot);
}

bool InteractWithRefreshmentPortalAction::Execute(Event event)
{
    GameObject* refreshmentPortal = FindNearestRitualObject(
        bot,
        {RITUAL_REFRESHMENT_PORTAL_RANK_1, RITUAL_REFRESHMENT_PORTAL_RANK_2},
        GetRitualSearchRange(bot));
    if (!refreshmentPortal)
        return false;

    if (bot->GetDistance(refreshmentPortal) > RITUAL_INTERACTION_DISTANCE)
    {
        return MoveTo(bot->GetMapId(), refreshmentPortal->GetPositionX(), refreshmentPortal->GetPositionY(), refreshmentPortal->GetPositionZ(), false, false);
    }

    refreshmentPortal->Use(bot);
    botAI->SetNextCheckDelay(kRitualDelayMs);
    return true;
}

bool InteractWithRefreshmentTableAction::isUseful()
{
    if (!CanUseRituals(bot))
        return false;

    if (!BotNeedsConjuredFoodOrWater(botAI, kConjuredFoodTargetCount))
        return false;

    return HasNearbyRefreshmentTable(bot);
}

bool InteractWithRefreshmentTableAction::Execute(Event event)
{
    GameObject* refreshmentTable = FindNearestRitualObject(
        bot,
        {RITUAL_REFRESHMENT_TABLE_RANK_1, RITUAL_REFRESHMENT_TABLE_RANK_2},
        GetRitualSearchRange(bot));
    if (!refreshmentTable)
        return false;

    if (bot->GetDistance(refreshmentTable) > RITUAL_INTERACTION_DISTANCE)
    {
        return MoveTo(bot->GetMapId(), refreshmentTable->GetPositionX(), refreshmentTable->GetPositionY(), refreshmentTable->GetPositionZ(), false, false);
    }

    refreshmentTable->Use(bot);
    botAI->SetNextCheckDelay(kRitualDelayMs);
    return true;
}

bool CheckConjuredItemsAction::isUseful()
{
    if (!CanUseRituals(bot))
        return false;

    bool needsHealthstone = BotNeedsHealthstone(botAI) && HasNearbySoulwell(bot);
    bool needsRefreshment = BotNeedsConjuredFoodOrWater(botAI, kConjuredFoodTargetCount) && HasNearbyRefreshmentTable(bot);
    return needsHealthstone || needsRefreshment;
}

bool CheckConjuredItemsAction::Execute(Event event)
{
    if (BotNeedsHealthstone(botAI) && HasNearbySoulwell(bot))
    {
        return InteractWithSoulwellAction(botAI).Execute(event);
    }

    if (BotNeedsConjuredFoodOrWater(botAI, kConjuredFoodTargetCount) && HasNearbyRefreshmentTable(bot))
    {
        return InteractWithRefreshmentTableAction(botAI).Execute(event);
    }

    return false;
}

bool SoulPortalAvailableTrigger::IsActive()
{
    if (!CanUseRituals(bot))
        return false;

    if (!BotNeedsHealthstone(botAI))
        return false;

    return HasNearbySoulPortal(bot);
}

bool SoulwellAvailableTrigger::IsActive()
{
    if (!CanUseRituals(bot))
        return false;

    if (bot->getClass() == CLASS_WARLOCK)
        return false;

    if (!BotNeedsHealthstone(botAI))
        return false;

    return HasNearbySoulwell(bot);
}

bool RefreshmentPortalAvailableTrigger::IsActive()
{
    if (!CanUseRituals(bot))
        return false;

    if (!BotNeedsConjuredFoodOrWater(botAI, kConjuredFoodTargetCount))
        return false;

    return HasNearbyRefreshmentPortal(bot);
}

bool RefreshmentTableAvailableTrigger::IsActive()
{
    if (!CanUseRituals(bot))
        return false;

    if (!BotNeedsConjuredFoodOrWater(botAI, kConjuredFoodTargetCount))
        return false;

    return HasNearbyRefreshmentTable(bot);
}

bool NeedsConjuredItemsTrigger::IsActive()
{
    if (!CanUseRituals(bot))
        return false;

    bool needsHealthstone = BotNeedsHealthstone(botAI) && HasNearbySoulwell(bot);
    bool needsRefreshment = BotNeedsConjuredFoodOrWater(botAI, kConjuredFoodTargetCount) && HasNearbyRefreshmentTable(bot);
    return needsHealthstone || needsRefreshment;
}