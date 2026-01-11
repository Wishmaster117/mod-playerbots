/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_RITUALACTIONS_H
#define _PLAYERBOT_RITUALACTIONS_H

#include "Action.h"
#include "MovementActions.h"
#include "Trigger.h"

#include <initializer_list>
#include <string>
#include <vector>

class GameObject;
class Player;
class PlayerbotAI;

constexpr float RITUAL_BATTLEGROUND_SEARCH_RANGE = 100.0f;
constexpr float RITUAL_DUNGEON_SEARCH_RANGE = 30.0f;
constexpr float RITUAL_INTERACTION_DISTANCE = 5.0f;
constexpr float RITUAL_GROUP_CHECK_RANGE = 100.0f;

constexpr uint32 RITUAL_OF_REFRESHMENT_RANK_1 = 43987;
constexpr uint32 RITUAL_OF_REFRESHMENT_RANK_2 = 58659;
constexpr uint32 RITUAL_OF_SOULS_RANK_1 = 29893;
constexpr uint32 RITUAL_OF_SOULS_RANK_2 = 58887;

constexpr uint32 RITUAL_SOUL_PORTAL_RANK_1 = 181622;
constexpr uint32 RITUAL_SOUL_PORTAL_RANK_2 = 193168;
constexpr uint32 RITUAL_SOUL_WELL_RANK_1 = 181621;
constexpr uint32 RITUAL_SOUL_WELL_RANK_2 = 193169;
constexpr uint32 RITUAL_SOUL_WELL_RANK_2_VARIANT_1 = 193170;
constexpr uint32 RITUAL_SOUL_WELL_RANK_2_VARIANT_2 = 193171;
constexpr uint32 RITUAL_REFRESHMENT_PORTAL_RANK_1 = 186811;
constexpr uint32 RITUAL_REFRESHMENT_PORTAL_RANK_2 = 193062;
constexpr uint32 RITUAL_REFRESHMENT_TABLE_RANK_1 = 186812;
constexpr uint32 RITUAL_REFRESHMENT_TABLE_RANK_2 = 193061;

constexpr uint32 RITUAL_MINOR_HEALTHSTONE = 5512;
constexpr uint32 RITUAL_LESSER_HEALTHSTONE = 5511;
constexpr uint32 RITUAL_MAJOR_HEALTHSTONE = 9421;
constexpr uint32 RITUAL_MINOR_HEALTHSTONE_ALT = 19004;
constexpr uint32 RITUAL_LESSER_HEALTHSTONE_ALT = 19005;
constexpr uint32 RITUAL_FEL_HEALTHSTONE = 36892;
constexpr uint32 RITUAL_DEMONIC_HEALTHSTONE = 22103;

float GetRitualSearchRange(Player* bot);
bool IsRitualMap(Player* bot);
bool CanUseRituals(Player* bot);
bool CanUseRituals(Player* bot, std::string* reason);
GameObject* FindNearestRitualObject(Player* bot, std::initializer_list<uint32> entries, float range);

bool HasHealthstone(Player const* player);
bool BotNeedsHealthstone(PlayerbotAI* botAI);
bool BotNeedsConjuredFoodOrWater(PlayerbotAI* botAI, uint32 desiredCount);
bool IsPrimaryRitualCaster(Player* bot, uint8 classId);

bool ShouldCastRitualOfRefreshment(PlayerbotAI* botAI);
bool ShouldCastRitualOfSouls(PlayerbotAI* botAI);

class InteractWithSoulPortalAction : public MovementAction
{
public:
    InteractWithSoulPortalAction(PlayerbotAI* botAI) : MovementAction(botAI, "interact with soul portal") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class InteractWithSoulwellAction : public MovementAction
{
public:
    InteractWithSoulwellAction(PlayerbotAI* botAI) : MovementAction(botAI, "interact with soulwell") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class InteractWithRefreshmentPortalAction : public MovementAction
{
public:
    InteractWithRefreshmentPortalAction(PlayerbotAI* botAI) : MovementAction(botAI, "interact with refreshment portal") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class InteractWithRefreshmentTableAction : public MovementAction
{
public:
    InteractWithRefreshmentTableAction(PlayerbotAI* botAI) : MovementAction(botAI, "interact with refreshment table") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class CheckConjuredItemsAction : public MovementAction
{
public:
    CheckConjuredItemsAction(PlayerbotAI* botAI) : MovementAction(botAI, "check conjured items") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class SoulPortalAvailableTrigger : public Trigger
{
public:
    SoulPortalAvailableTrigger(PlayerbotAI* botAI) : Trigger(botAI, "soul portal available") {}
    bool IsActive() override;
};

class SoulwellAvailableTrigger : public Trigger
{
public:
    SoulwellAvailableTrigger(PlayerbotAI* botAI) : Trigger(botAI, "soulwell available") {}
    bool IsActive() override;
};

class RefreshmentPortalAvailableTrigger : public Trigger
{
public:
    RefreshmentPortalAvailableTrigger(PlayerbotAI* botAI) : Trigger(botAI, "refreshment portal available") {}
    bool IsActive() override;
};

class RefreshmentTableAvailableTrigger : public Trigger
{
public:
    RefreshmentTableAvailableTrigger(PlayerbotAI* botAI) : Trigger(botAI, "refreshment table available") {}
    bool IsActive() override;
};

class NeedsConjuredItemsTrigger : public Trigger
{
public:
    NeedsConjuredItemsTrigger(PlayerbotAI* botAI) : Trigger(botAI, "needs conjured items") {}
    bool IsActive() override;
};

#endif