/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NAXXBOSSHELPER_H
#define PLAYERBOTS_NAXXBOSSHELPER_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "AiObject.h"
#include "AiObjectContext.h"
#include "EventMap.h"
#include "Log.h"
#include "NamedObjectContext.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "ScriptedCreature.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "Timer.h"
#include "NaxxSpellIds.h"

const uint32 NAXX_MAP_ID = 533;

template <class BossAiType>
class GenericBossHelper : public AiObject
{
public:
    GenericBossHelper(PlayerbotAI* botAI, std::string name) : AiObject(botAI), _name(name) {}
    virtual bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            _unit = nullptr;

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            _unit = nullptr;

        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", _name);
            if (!_unit)
                return false;

            _target = _unit->ToCreature();
            if (!_target)
                return false;

            _ai = dynamic_cast<BossAiType*>(_target->GetAI());
            if (!_ai)
                return false;

            _event_map = &_ai->events;
            if (!_event_map)
                return false;
        }
        if (!_event_map)
            return false;

        _timer = getMSTime();
        return true;
    }
    virtual void Reset()
    {
        _unit = nullptr;
        _target = nullptr;
        _ai = nullptr;
        _event_map = nullptr;
        _timer = 0;
    }

protected:
    std::string _name;
    Unit* _unit = nullptr;
    Creature* _target = nullptr;
    BossAiType* _ai = nullptr;
    EventMap* _event_map = nullptr;
    uint32 _timer = 0;
};

class KelthuzadBossHelper : public AiObject
{
public:
    explicit KelthuzadBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    const std::pair<float, float> center = {3716.19f, -5106.58f};
    const std::pair<float, float> tank_pos = {3709.19f, -5104.86f};
    const std::pair<float, float> assist_tank_pos = {3746.05f, -5112.74f};

    static constexpr float ROOM_MIN_RADIUS = 6.0f;
    static constexpr float ROOM_MAX_RADIUS = 24.0f;
    static constexpr float DETONATE_MIN_RADIUS = 20.0f;
    static constexpr float DETONATE_MAX_RADIUS = 24.0f;
    static constexpr float TANK_HOLD_MAX_RADIUS = 20.0f;
    static constexpr float PHASE1_TANK_MAX_RADIUS = 16.0f;
    static constexpr float PHASE1_TANK_HOLD_RADIUS = 12.0f;

    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "kel'thuzad");

        return _unit != nullptr;
    }

    bool IsPhaseOne() const { return _unit && _unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE); }
    bool IsPhaseTwo() const { return _unit && !_unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE); }
    Unit* GetBoss() const { return _unit; }

    bool IsBossCasting(uint32 spellId) const
    {
        if (!_unit)
            return false;

        Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
            spell = _unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

        SpellInfo const* info = spell ? spell->GetSpellInfo() : nullptr;
        return info && info->Id == spellId;
    }

    uint32 GetRangedCount() const
    {
        Group* group = bot->GetGroup();
        if (!group)
            return botAI->IsRanged(bot) ? 1 : 0;

        uint32 count = 0;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member->IsAlive() && botAI->IsRanged(member))
                ++count;
        }
        return count;
    }

    void ClampToRoom(float& x, float& y, float minRadius = ROOM_MIN_RADIUS, float maxRadius = ROOM_MAX_RADIUS) const
    {
        float dx = x - center.first;
        float dy = y - center.second;
        float radiusSquared = dx * dx + dy * dy;
        if (radiusSquared < 0.0001f)
        {
            x = center.first + minRadius;
            y = center.second;
            return;
        }

        float const radius = std::sqrt(radiusSquared);
        float const clampedRadius = std::clamp(radius, minRadius, maxRadius);
        x = center.first + dx / radius * clampedRadius;
        y = center.second + dy / radius * clampedRadius;
    }

    bool IsWithinRoom(WorldObject const* object, float maxRadius = ROOM_MAX_RADIUS) const
    {
        return object && object->GetDistance2d(center.first, center.second) <= maxRadius;
    }

    std::pair<float, float> GetMainTankHoldPosition() const
    {
        float x = tank_pos.first;
        float y = tank_pos.second;
        ClampToRoom(x, y, ROOM_MIN_RADIUS, TANK_HOLD_MAX_RADIUS);
        return {x, y};
    }

    std::pair<float, float> GetAssistTankHoldPosition() const
    {
        float x = assist_tank_pos.first;
        float y = assist_tank_pos.second;
        ClampToRoom(x, y, ROOM_MIN_RADIUS, TANK_HOLD_MAX_RADIUS);
        return {x, y};
    }

    void ComputeRangedSpreadPosition(uint32 index, uint32 total, float& outX, float& outY) const
    {
        total = std::max<uint32>(1, total);

        float const radii[3] = {18.0f, 21.0f, 24.0f};
        uint32 ringSizes[3] = {0, 0, 0};
        uint32 rings = 1;

        if (total <= 10)
        {
            ringSizes[0] = total;
        }
        else if (total <= 18)
        {
            rings = 2;
            ringSizes[0] = (total + 1) / 2;
            ringSizes[1] = total - ringSizes[0];
        }
        else
        {
            rings = 3;
            ringSizes[0] = (total + 2) / 3;
            ringSizes[1] = (total + 1) / 3;
            ringSizes[2] = total - ringSizes[0] - ringSizes[1];
        }

        uint32 ring = 0;
        uint32 localIndex = index;
        for (uint32 candidate = 0; candidate < rings; ++candidate)
        {
            if (localIndex < ringSizes[candidate])
            {
                ring = candidate;
                break;
            }
            localIndex -= ringSizes[candidate];
        }

        uint32 const slots = std::max<uint32>(1, ringSizes[ring]);
        float angle = 2.0f * float(M_PI) * (float(localIndex) / float(slots));
        angle += float(ring) * (float(M_PI) / 8.0f);

        outX = center.first + std::cos(angle) * radii[ring];
        outY = center.second + std::sin(angle) * radii[ring];
        ClampToRoom(outX, outY);
    }

    Player* GetPlayerWithAura(uint32 spellId) const
    {
        if (bot->HasAura(spellId))
            return bot;

        Group* group = bot->GetGroup();
        if (!group)
            return nullptr;

        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member->IsAlive() && member->HasAura(spellId))
                return member;
        }
        return nullptr;
    }

    bool HasAuraInGroup(uint32 spellId) const { return GetPlayerWithAura(spellId) != nullptr; }
    bool HasDetonateMana(Player* player) const { return player && player->HasAura(NaxxSpellIds::DetonateMana); }
    bool HasChains(Player* player) const { return player && player->HasAura(NaxxSpellIds::ChainsOfKelthuzad); }

    bool IsGuardian(Unit* unit) const
    {
        if (!unit)
            return false;
        if (Creature* creature = unit->ToCreature())
            return creature->GetEntry() == NaxxSpellIds::KelthuzadGuardianEntry;
        return false;
    }

    std::vector<Unit*> GetGuardians() const
    {
        std::vector<Unit*> guardians;

        auto collect = [&](GuidVector const& guids)
        {
            for (ObjectGuid const& guid : guids)
            {
                Unit* unit = botAI->GetUnit(guid);
                if (!unit || !unit->IsAlive() || !IsGuardian(unit))
                    continue;
                if (!IsWithinRoom(unit, ROOM_MAX_RADIUS + 4.0f))
                    continue;
                if (std::find(guardians.begin(), guardians.end(), unit) == guardians.end())
                    guardians.push_back(unit);
            }
        };

        collect(context->GetValue<GuidVector>("possible targets")->Get());
        collect(context->GetValue<GuidVector>("attackers")->Get());
        return guardians;
    }

    bool AllGuardiansOnAssistTank(Player* assistTank) const
    {
        if (!assistTank)
            return false;

        std::vector<Unit*> const guardians = GetGuardians();
        if (guardians.empty())
            return false;

        for (Unit* guardian : guardians)
        {
            if (guardian->GetVictim() != assistTank)
                return false;
        }
        return true;
    }

    Unit* GetGuardianToPickup(Player* assistTank) const
    {
        if (!assistTank)
            return nullptr;

        std::vector<Unit*> const guardians = GetGuardians();
        Unit* best = nullptr;
        float bestDistance = std::numeric_limits<float>::max();

        for (Unit* guardian : guardians)
        {
            if (guardian->GetVictim() == assistTank)
                continue;

            float const distance = assistTank->GetDistance2d(guardian);
            if (!best || distance < bestDistance)
            {
                best = guardian;
                bestDistance = distance;
            }
        }

        if (best)
            return best;

        for (Unit* guardian : guardians)
        {
            float const distance = assistTank->GetDistance2d(guardian);
            if (!best || distance < bestDistance)
            {
                best = guardian;
                bestDistance = distance;
            }
        }
        return best;
    }

    Unit* GetAnyShadowFissure() const
    {
        Unit* nearest = nullptr;
        float nearestDistance = std::numeric_limits<float>::max();
        GuidVector const units = context->GetValue<GuidVector>("nearest triggers")->Get();
        for (ObjectGuid const& guid : units)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (!unit || !unit->IsAlive())
                continue;
            if (!botAI->EqualLowercaseName(unit->GetName(), "shadow fissure"))
                continue;

            float const distance = bot->GetDistance2d(unit);
            if (!nearest || distance < nearestDistance)
            {
                nearest = unit;
                nearestDistance = distance;
            }
        }
        return nearest;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class RazuviousBossHelper : public AiObject
{
public:
    RazuviousBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "instructor razuvious");

        return _unit != nullptr;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class SapphironBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos = {3512.07f, -5274.06f};
    const std::pair<float, float> center = {3517.31f, -5253.74f};
    const float GENERIC_HEIGHT = 137.29f;

    SapphironBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    Unit* GetBoss() const { return _unit; }

    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "sapphiron");
            if (!_unit)
                return false;
        }

        bool const nowFlying = _unit->IsFlying();
        if (_wasFlying && !nowFlying)
            _lastLandMs = getMSTime();

        _wasFlying = nowFlying;

        bool const hasIcebolt = HasIceboltInGroup();
        if (hasIcebolt && !_hadIcebolt)
            _firstIceboltMs = getMSTime();
        if (!nowFlying)
        {
            _hadIcebolt = false;
            _firstIceboltMs = 0;
        }
        else
        {
            _hadIcebolt = hasIcebolt;
        }

        return true;
    }

    bool IsPhaseGround() const { return _unit && !_unit->IsFlying(); }
    bool IsPhaseFlight() const { return _unit && _unit->IsFlying(); }

    bool JustLanded() const
    {
        if (!_lastLandMs)
            return false;

        return getMSTime() - _lastLandMs <= PositionTimeAfterLandedMs;
    }

    bool IsIceboltTarget(Unit* unit) const
    {
        if (!unit)
            return false;

        return NaxxSpellIds::HasAnyAura(unit, {NaxxSpellIds::Icebolt10, NaxxSpellIds::Icebolt25}) ||
               botAI->HasAura("icebolt", unit, false, false, -1, true);
    }

    void GetIceboltTargets(std::vector<Player*>& targets) const
    {
        targets.clear();

        Group* group = bot->GetGroup();
        if (!group)
            return;

        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || !member->IsAlive() || member->GetMapId() != bot->GetMapId())
                continue;

            if (IsIceboltTarget(member))
                targets.push_back(member);
        }
    }

    bool HasIceboltInGroup() const
    {
        std::vector<Player*> icebolts;
        GetIceboltTargets(icebolts);
        return !icebolts.empty();
    }

    bool WaitForExplosion() const
    {
        if (!IsPhaseFlight())
            return false;

        if (HasIceboltInGroup() || IsBreathCasting())
            return true;

        if (!_firstIceboltMs)
            return false;

        // AzerothCore casts the breath shortly after the final Icebolt and explodes 8.5 seconds later.
        return getMSTime() - _firstIceboltMs <= BreathSafetyWindowMs;
    }

    bool HasLifeDrainInGroup() const
    {
        Group* group = bot->GetGroup();
        if (!group)
            return false;

        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || !member->IsAlive() || member->GetMapId() != bot->GetMapId())
                continue;

            if (NaxxSpellIds::HasAnyAura(member, {NaxxSpellIds::LifeDrain}) || botAI->HasAura("life drain", member))
                return true;
        }

        return false;
    }

    bool FindPosToAvoidChill(std::vector<float>& dest)
    {
        Aura* aura = NaxxSpellIds::GetAnyAura(bot, {NaxxSpellIds::Chill10, NaxxSpellIds::Chill25});
        if (!aura)
            aura = botAI->GetAura("chill", bot);
        if (!aura)
            return false;

        WorldObject* source = aura->GetDynobjOwner();
        if (!source)
            source = botAI->GetUnit(aura->GetCasterGUID());
        if (!source)
            return false;

        float angle = source->GetAngle(bot);
        if (bot->GetExactDist2d(source) < 0.1f)
            angle = bot->GetOrientation();

        float const distance = botAI->IsRanged(bot) || botAI->IsHeal(bot) ? 8.0f : 6.0f;
        dest = {source->GetPositionX() + std::cos(angle) * distance,
                source->GetPositionY() + std::sin(angle) * distance,
                bot->GetPositionZ()};
        return true;
    }

private:
    bool IsBreathCasting() const
    {
        if (!_unit || !_unit->HasUnitState(UNIT_STATE_CASTING))
            return false;

        Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
            spell = _unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        if (!spell)
            return false;

        return NaxxSpellIds::MatchesAnySpellId(
            spell->GetSpellInfo(), {NaxxSpellIds::FrostMissile, NaxxSpellIds::FrostExplosion});
    }

    void Reset()
    {
        _unit = nullptr;
        _wasFlying = false;
        _hadIcebolt = false;
        _lastLandMs = 0;
        _firstIceboltMs = 0;
    }

    static constexpr uint32 PositionTimeAfterLandedMs = 5000;
    static constexpr uint32 BreathSafetyWindowMs = 25000;

    Unit* _unit = nullptr;
    bool _wasFlying = false;
    bool _hadIcebolt = false;
    uint32 _lastLandMs = 0;
    uint32 _firstIceboltMs = 0;
};

class GluthBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos25 = {3331.48f, -3109.06f};
    const std::pair<float, float> mainTankPos10 = {3278.29f, -3162.06f};
    const std::pair<float, float> beforeDecimatePos = {3267.34f, -3175.68f};
    const std::pair<float, float> leftSlowDownPos = {3290.68f, -3141.65f};
    const std::pair<float, float> rightSlowDownPos = {3300.78f, -3151.98f};
    const std::pair<float, float> rangedPos = {3301.45f, -3139.29f};
    const std::pair<float, float> healPos = {3303.09f, -3135.24f};

    const float decimatedZombiePct = 10.0f;
    GluthBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    Unit* GetBoss() const { return _unit; }

    bool IsZombieKiter(Player* player) const
    {
        if (!player)
            return false;

        if (botAI->IsAssistTankOfIndex(player, 1))
            return true;

        return player->GetRaidDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL &&
               botAI->GetClassIndex(player, CLASS_HUNTER) == 0;
    }

    bool IsSlowdownHunter(Player* player) const
    {
        if (!player || player->getClass() != CLASS_HUNTER)
            return false;

        uint32 const hunterIndex = botAI->GetClassIndex(player, CLASS_HUNTER);
        if (player->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
            return hunterIndex <= 1;

        return hunterIndex == 0;
    }

    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "gluth");
            if (!_unit)
                return false;
        }
        if (_unit->IsInCombat())
        {
            if (_combat_start_ms == 0)
                _combat_start_ms = getMSTime();
        }
        else
            _combat_start_ms = 0;

        return true;
    }
    bool BeforeDecimate()
    {
        if (!_unit || !_unit->HasUnitState(UNIT_STATE_CASTING))
            return false;

        Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
            spell = _unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

        if (!spell)
            return false;

        SpellInfo const* info = spell->GetSpellInfo();
        if (!info)
            return false;

        if (NaxxSpellIds::MatchesAnySpellId(
                info, {NaxxSpellIds::Decimate10, NaxxSpellIds::Decimate25, NaxxSpellIds::Decimate25Alt}))
            return true;

        // Fallback to name for custom spell data.
        return info->SpellName[LOCALE_enUS] && botAI->EqualLowercaseName(info->SpellName[LOCALE_enUS], "decimate");
    }
    bool JustStartCombat() const { return _combat_start_ms != 0 && getMSTime() - _combat_start_ms < 10000; }
    bool IsZombieChow(Unit* unit) const { return unit && botAI->EqualLowercaseName(unit->GetName(), "zombie chow"); }

private:
    void Reset()
    {
        _unit = nullptr;
        _combat_start_ms = 0;
    }

    Unit* _unit = nullptr;
    uint32 _combat_start_ms = 0;
};

class LoathebBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos = {2877.57f, -3967.00f};
    const std::pair<float, float> rangePos = {2896.96f, -3980.61f};
    LoathebBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    Unit* GetBoss() const { return _unit; }

    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "loatheb");

        return _unit != nullptr;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class NothBossHelper : public AiObject
{
public:
    NothBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "noth the plaguebringer");

        if (!_unit)
            return false;

        if (_unit->HasUnitState(UNIT_STATE_CASTING))
        {
            Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
            if (!spell)
                spell = _unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

            SpellInfo const* info = spell ? spell->GetSpellInfo() : nullptr;
            bool isBlink =
                NaxxSpellIds::MatchesAnySpellId(info, {NaxxSpellIds::NothBlink, NaxxSpellIds::NothCripple});
            if (!isBlink && info && info->SpellName[LOCALE_enUS])
            {
                isBlink = botAI->EqualLowercaseName(info->SpellName[LOCALE_enUS], "blink") ||
                          botAI->EqualLowercaseName(info->SpellName[LOCALE_enUS], "cripple");
            }

            if (isBlink)
                _lastBlinkMs = getMSTime();
        }

        return true;
    }

    bool IsBalconyPhase() const
    {
        return _unit && _unit->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
    }

    bool IsBlinkWindow() const
    {
        return _lastBlinkMs != 0 && getMSTime() - _lastBlinkMs < 3000;
    }

    bool HasCurseInGroup() const
    {
        GuidVector const members = AI_VALUE(GuidVector, "group members");
        for (ObjectGuid const& guid : members)
        {
            Unit* member = botAI->GetUnit(guid);
            if (!member || !member->IsAlive())
                continue;

            if (NaxxSpellIds::HasAnyAura(member, {NaxxSpellIds::CurseOfThePlaguebringer}) ||
                botAI->HasAura("curse of the plaguebringer", member))
            {
                return true;
            }
        }

        return false;
    }

    Player* GetAliveAssistTank() const
    {
        GuidVector const members = AI_VALUE(GuidVector, "group members");
        for (ObjectGuid const& guid : members)
        {
            Unit* member = botAI->GetUnit(guid);
            Player* player = member ? member->ToPlayer() : nullptr;
            if (player && player->IsAlive() && botAI->IsAssistTank(player))
                return player;
        }

        return nullptr;
    }

private:
    void Reset()
    {
        _unit = nullptr;
        _lastBlinkMs = 0;
    }

    Unit* _unit = nullptr;
    uint32 _lastBlinkMs = 0;
};

class FourHorsemenBossHelper : public AiObject
{
public:
    const float posZ = 241.27f;
    const std::pair<float, float> attractPos[2] = {{2502.03f, -2910.90f},
                                                   {2484.61f, -2947.07f}};  // Sir Zeliek, Lady Blaumeux

    FourHorsemenBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    bool UpdateBossAI()
    {
        if (_sir && (!_sir->IsInWorld() || !_sir->IsAlive()))
            _sir = nullptr;
        if (_lady && (!_lady->IsInWorld() || !_lady->IsAlive()))
            _lady = nullptr;

        if (!_sir)
            _sir = AI_VALUE2(Unit*, "find target", "sir zeliek");
        if (!_lady)
            _lady = AI_VALUE2(Unit*, "find target", "lady blaumeux");

        bool const encounterActive = bot->IsInCombat() || (_sir && _sir->IsInCombat()) || (_lady && _lady->IsInCombat());
        if (!encounterActive)
        {
            _lastSwitchMs = 0;
            _positionInitialized = false;
        }

        return _sir || _lady;
    }

    bool IsAttracter(Player* player)
    {
        Difficulty const difficulty = player->GetRaidDifficulty();
        if (difficulty == RAID_DIFFICULTY_25MAN_NORMAL)
        {
            return botAI->IsAssistRangedDpsOfIndex(player, 0) || botAI->IsAssistHealOfIndex(player, 0) ||
                   botAI->IsAssistHealOfIndex(player, 1) || botAI->IsAssistHealOfIndex(player, 2);
        }

        return botAI->IsAssistRangedDpsOfIndex(player, 0) || botAI->IsAssistHealOfIndex(player, 0);
    }

    void CalculatePosToGo(Player* player)
    {
        if (!_positionInitialized)
        {
            posToGo = InitialAttractSide(player);
            _positionInitialized = true;
            _lastSwitchMs = getMSTime();
        }

        Unit* currentTarget = CurrentAttackTarget();
        Unit* otherTarget = posToGo == 0 ? _lady : _sir;
        if ((!currentTarget || !currentTarget->IsAlive()) && otherTarget && otherTarget->IsAlive())
        {
            posToGo = 1 - posToGo;
            _lastSwitchMs = getMSTime();
            return;
        }

        if (!currentTarget || !otherTarget || !otherTarget->IsAlive())
            return;

        uint32 const markSpell = posToGo == 0 ? NaxxSpellIds::MarkOfZeliek : NaxxSpellIds::MarkOfBlaumeux;
        Aura* mark = player->GetAura(markSpell);
        if (!mark)
            return;

        uint32 const now = getMSTime();
        if (mark->GetStackAmount() >= SafeMarkStacks && now - _lastSwitchMs >= MinSwitchIntervalMs)
        {
            posToGo = 1 - posToGo;
            _lastSwitchMs = now;
        }
    }

    std::pair<float, float> CurrentAttractPos() const
    {
        bool const raid25 = bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
        float posX = attractPos[posToGo].first;
        float posY = attractPos[posToGo].second;

        if (posToGo == 1 && raid25)
        {
            posX -= 4.5f;
            posY += 4.5f;
        }

        return {posX, posY};
    }

    Unit* CurrentAttackTarget() const
    {
        Unit* selected = posToGo == 0 ? _sir : _lady;
        if (selected && selected->IsAlive())
            return selected;

        Unit* fallback = posToGo == 0 ? _lady : _sir;
        return fallback && fallback->IsAlive() ? fallback : nullptr;
    }

private:
    int InitialAttractSide(Player* player) const
    {
        bool const raid25 = player->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;

        // Split the rear team from the first pull. The marked stacks then drive every later swap.
        if (botAI->IsAssistRangedDpsOfIndex(player, 0))
            return 1;
        if (raid25 && botAI->IsAssistHealOfIndex(player, 1))
            return 1;

        return 0;
    }

    static constexpr uint8 SafeMarkStacks = 3;
    static constexpr uint32 MinSwitchIntervalMs = 20000;

    Unit* _sir = nullptr;
    Unit* _lady = nullptr;
    uint32 _lastSwitchMs = 0;
    bool _positionInitialized = false;
    int posToGo = 0;
};

class ThaddiusBossHelper : public AiObject
{
public:
    const std::pair<float, float> tankPosFeugen = {3522.94f, -3002.60f};
    const std::pair<float, float> tankPosStalagg = {3436.14f, -2919.98f};
    const std::pair<float, float> rangedPosFeugen = {3500.45f, -2997.92f};
    const std::pair<float, float> rangedPosStalagg = {3441.01f, -2942.04f};
    const float tankPosZ = 312.61f;

    ThaddiusBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    bool UpdateBossAI()
    {
        ValidateUnit(_unit);
        ValidateUnit(feugen);
        ValidateUnit(stalagg);

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "thaddius");
        if (!feugen)
            feugen = AI_VALUE2(Unit*, "find target", "feugen");
        if (!stalagg)
            stalagg = AI_VALUE2(Unit*, "find target", "stalagg");

        return _unit || feugen || stalagg;
    }

    bool IsPhasePet() const { return IsAlive(feugen) || IsAlive(stalagg); }

    bool IsPetPhaseEngaged() const
    {
        return bot->IsInCombat() || (feugen && feugen->IsInCombat()) || (stalagg && stalagg->IsInCombat());
    }

    bool IsPhaseTransition() const
    {
        if (IsPhasePet())
            return false;

        return IsAlive(_unit) && _unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
    }

    bool IsPhaseThaddius() const
    {
        return IsAlive(_unit) && !IsPhasePet() && !_unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
    }

    Unit* GetFeugen() const { return IsAlive(feugen) ? feugen : nullptr; }
    Unit* GetStalagg() const { return IsAlive(stalagg) ? stalagg : nullptr; }

    Unit* GetAssignedPetForBot()
    {
        Unit* aliveFeugen = GetFeugen();
        Unit* aliveStalagg = GetStalagg();
        if (!aliveFeugen)
            return aliveStalagg;
        if (!aliveStalagg)
            return aliveFeugen;

        if (botAI->IsMainTank(bot))
            return aliveStalagg;
        if (botAI->IsAssistTank(bot))
            return aliveFeugen;

        int32 const roleIndex = GetRoleIndex(bot);
        return roleIndex % 2 == 0 ? aliveStalagg : aliveFeugen;
    }

    std::pair<float, float> PetPhaseGetPosForTank(Unit* pet) const
    {
        return pet == feugen ? tankPosFeugen : tankPosStalagg;
    }

    std::pair<float, float> PetPhaseGetPosForRanged(Unit* pet) const
    {
        return pet == feugen ? rangedPosFeugen : rangedPosStalagg;
    }

private:
    static bool IsAlive(Unit* unit)
    {
        return unit && unit->IsInWorld() && unit->IsAlive();
    }

    static void ValidateUnit(Unit*& unit)
    {
        if (unit && (!unit->IsInWorld() || !unit->IsAlive()))
            unit = nullptr;
    }

    int32 GetRoleIndex(Player* player) const
    {
        Group* group = player ? player->GetGroup() : nullptr;
        if (!group)
            return 0;

        std::vector<Player*> sameRole;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || !member->IsAlive() || member->GetMapId() != player->GetMapId())
                continue;

            bool const same = botAI->IsHeal(player) ? botAI->IsHeal(member) :
                              (!botAI->IsTank(member) && !botAI->IsHeal(member));
            if (same)
                sameRole.push_back(member);
        }

        std::sort(sameRole.begin(), sameRole.end(), [](Player const* left, Player const* right)
        {
            return left->GetGUID().GetRawValue() < right->GetGUID().GetRawValue();
        });

        for (size_t index = 0; index < sameRole.size(); ++index)
        {
            if (sameRole[index] == player)
                return static_cast<int32>(index);
        }

        return 0;
    }

    Unit* _unit = nullptr;
    Unit* feugen = nullptr;
    Unit* stalagg = nullptr;
};

#endif
