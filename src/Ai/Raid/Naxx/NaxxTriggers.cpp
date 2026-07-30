/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxTriggers.h"

#include "Playerbots.h"
#include "NaxxSpellIds.h"
#include "Timer.h"
#include "Trigger.h"

namespace
{
    bool HasGrobbulusInjection(PlayerbotAI* botAI, Unit* unit)
    {
        return unit && (NaxxSpellIds::HasAnyAura(unit, {NaxxSpellIds::MutatingInjection}) ||
                        botAI->HasAura("mutating injection", unit, false, false, -1, true));
    }

    bool IsGrobbulusCloud(PlayerbotAI* botAI, Unit* unit)
    {
        if (!unit || !unit->IsAlive())
            return false;

        return unit->HasAura(NaxxSpellIds::PoisonCloudDamageAura) ||
               botAI->EqualLowercaseName(unit->GetName(), "poison cloud");
    }
}

bool MutatingInjectionTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss || !boss->IsAlive() || (!bot->IsInCombat() && !boss->IsInCombat()))
        return false;

    return HasGrobbulusInjection(botAI, bot);
}

bool MutatingInjectionMeleeTrigger::IsActive()
{
    return MutatingInjectionTrigger::IsActive() && !botAI->IsRanged(bot);
}

bool MutatingInjectionRangedTrigger::IsActive()
{
    return MutatingInjectionTrigger::IsActive() && botAI->IsRanged(bot);
}

bool AuraRemovedTrigger::IsActive()
{
    bool const check = botAI->HasAura(name, bot, false, false, -1, true);
    bool const result = prev_check && !check;
    prev_check = check;
    return result;
}

bool MutatingInjectionRemovedTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    bool const hasInjection = HasGrobbulusInjection(botAI, bot);

    if (!boss || !boss->IsAlive() || (!bot->IsInCombat() && !boss->IsInCombat()))
    {
        hadInjection = false;
        return false;
    }

    bool const removed = hadInjection && !hasInjection;
    hadInjection = hasInjection;
    return removed;
}

bool GrobbulusCloudTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss || !boss->IsAlive() || (!bot->IsInCombat() && !boss->IsInCombat()))
    {
        combatStarted = false;
        nextRotationMs = 0;
        lastRotationMs = 0;
        lastCloudGuid = 0;
        return false;
    }

    if (!botAI->IsMainTank(bot) || !AI_VALUE2(bool, "has aggro", "boss target"))
        return false;

    uint32 const now = getMSTime();
    if (!combatStarted)
    {
        combatStarted = true;
        nextRotationMs = now + CloudFallbackDelayMs;
        return false;
    }

    Unit* newestCloud = nullptr;
    float nearestBossDistance = 18.0f;
    GuidVector const triggers = AI_VALUE(GuidVector, "nearest triggers");
    for (ObjectGuid const& guid : triggers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!IsGrobbulusCloud(botAI, unit))
            continue;

        float const bossDistance = boss->GetDistance2d(unit);
        if (bossDistance <= nearestBossDistance)
        {
            newestCloud = unit;
            nearestBossDistance = bossDistance;
        }
    }

    if (newestCloud)
    {
        uint64 const guid = newestCloud->GetGUID().GetRawValue();
        if (guid != lastCloudGuid)
        {
            lastCloudGuid = guid;
            nextRotationMs = now + CloudRotationPeriodMs;

            if (lastRotationMs == 0 || now - lastRotationMs >= 5000)
            {
                lastRotationMs = now;
                return true;
            }
        }
    }

    if (nextRotationMs != 0 && now >= nextRotationMs && (lastRotationMs == 0 || now - lastRotationMs >= 10000))
    {
        lastRotationMs = now;
        nextRotationMs = now + CloudRotationPeriodMs;
        return true;
    }

    return false;
}

bool GrobbulusTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    return boss && boss->IsAlive() && (bot->IsInCombat() || boss->IsInCombat());
}

bool HeiganMeleeTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!boss || !boss->IsAlive())
        return false;

    if (!bot->IsInCombat() && !boss->IsInCombat())
        return false;

    return !botAI->IsRanged(bot);
}

bool HeiganRangedTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!boss || !boss->IsAlive())
        return false;

    if (!bot->IsInCombat() && !boss->IsInCombat())
        return false;

    return botAI->IsRanged(bot);
}

bool HeiganDecrepitFeverTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!boss || !boss->IsAlive())
        return false;

    if (!bot->IsInCombat() && !boss->IsInCombat())
        return false;

    if (boss->GetPositionZ() >= 270.0f)
        return false;

    switch (bot->getClass())
    {
        case CLASS_PALADIN:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            break;
        default:
            return false;
    }

    float range = botAI->GetRange("heal");
    if (Group* group = bot->GetGroup())
    {
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member->IsAlive() && member->GetMapId() == bot->GetMapId() &&
                member->HasAura(NaxxSpellIds::DecrepitFever) && bot->IsWithinDistInMap(member, range))
            {
                return true;
            }
        }
        return false;
    }

    return bot->HasAura(NaxxSpellIds::DecrepitFever);
}

bool RazuviousTankTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    Difficulty const difficulty = bot->GetRaidDifficulty();
    if (difficulty == RAID_DIFFICULTY_10MAN_NORMAL)
        return botAI->IsMainTank(bot) || botAI->IsAssistTank(bot);

    int32 const priestIndex = botAI->GetClassIndex(bot, CLASS_PRIEST);
    return bot->getClass() == CLASS_PRIEST && priestIndex >= 0 && priestIndex < 2;
}

bool RazuviousNontankTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    Difficulty const difficulty = bot->GetRaidDifficulty();
    if (difficulty == RAID_DIFFICULTY_10MAN_NORMAL)
        return !botAI->IsMainTank(bot) && !botAI->IsAssistTank(bot);

    int32 const priestIndex = botAI->GetClassIndex(bot, CLASS_PRIEST);
    return bot->getClass() != CLASS_PRIEST || priestIndex < 0 || priestIndex >= 2;
}

bool FourHorsemenAttractorsTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsAttracter(bot);
}

bool FourHorsemenExceptAttractorsTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return !helper.IsAttracter(bot);
}

bool SapphironGroundTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhaseGround();
}

bool SapphironFlightTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhaseFlight();
}

bool GluthTrigger::IsActive() { return helper.UpdateBossAI(); }

bool GluthMainTankMortalWoundTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    if (!botAI->IsAssistTankOfIndex(bot, 0))
        return false;

    Unit* mt = AI_VALUE(Unit*, "main tank");
    if (!mt)
        return false;

    Aura* aura = NaxxSpellIds::GetAnyAura(mt, {NaxxSpellIds::MortalWound10, NaxxSpellIds::MortalWound25});
    if (!aura)
    {
        // Fallback to name for custom spell data.
        aura = botAI->GetAura("mortal wound", mt, false, true);
    }
    if (!aura || aura->GetStackAmount() < 5)
        return false;

    return true;
}

bool KelthuzadTrigger::IsActive() { return helper.UpdateBossAI(); }

bool AnubrekhanTrigger::IsActive() {
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
        return false;

    return true;
}

bool FaerlinaTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!boss)
        return false;

    return bot->IsInCombat() || boss->IsInCombat();
}

bool FaerlinaFrenzyTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!boss || (!bot->IsInCombat() && !boss->IsInCombat()))
        return false;

    if (boss->HasAura(NaxxSpellIds::FaerlinaWidowsEmbrace))
        return false;

    return boss->HasAura(NaxxSpellIds::FaerlinaFrenzy);
}

bool MaexxnaTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "maexxna");
    if (!boss)
        return false;

    return !botAI->IsTank(bot);
}

bool MaexxnaWebWrapTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "maexxna");
    if (!boss || (!bot->IsInCombat() && !boss->IsInCombat()))
        return false;

    if (botAI->IsTank(bot) || botAI->IsHeal(bot) || !botAI->IsRanged(bot))
        return false;

    if (Group* group = bot->GetGroup())
    {
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member->IsAlive() && member->HasAura(NaxxSpellIds::MaexxnaWebWrapStun))
                return true;
        }
    }

    GuidVector const targets = AI_VALUE(GuidVector, "possible targets no los");
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && unit->GetEntry() == NaxxSpellIds::MaexxnaWebWrapEntry)
            return true;
    }

    return false;
}

bool MaexxnaSpiderlingsTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "maexxna");
    if (!boss || (!bot->IsInCombat() && !boss->IsInCombat()))
        return false;

    if (!botAI->IsTank(bot) || botAI->IsMainTank(bot))
        return false;

    GuidVector const attackers = AI_VALUE(GuidVector, "attackers");
    for (ObjectGuid const& guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && unit->GetEntry() == NaxxSpellIds::MaexxnaSpiderlingEntry)
            return true;
    }

    return false;
}

bool GothikMoveToAssignedSideTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    return boss && bot->GetDistance(boss) <= 160.0f &&
           (boss->GetPositionZ() > 280.0f || boss->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE));
}

bool GothikChooseTargetTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    return boss && (bot->IsInCombat() || boss->IsInCombat());
}

bool NothTrigger::IsActive()
{
    return helper.UpdateBossAI();
}

bool NothCurseTrigger::IsActive()
{
    if (!helper.UpdateBossAI() || !helper.HasCurseInGroup())
        return false;

    switch (bot->getClass())
    {
        case CLASS_DRUID:
        case CLASS_MAGE:
        case CLASS_SHAMAN:
            return true;
        default:
            return false;
    }
}

bool PatchwerkTankTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "patchwerk");
    return boss && (bot->IsInCombat() || boss->IsInCombat()) && botAI->IsTank(bot);
}

bool PatchwerkRangedTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "patchwerk");
    return boss && (bot->IsInCombat() || boss->IsInCombat()) && !botAI->IsTank(bot) && botAI->IsRanged(bot);
}

bool PatchwerkNonTankTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "patchwerk");
    return boss && (bot->IsInCombat() || boss->IsInCombat()) && !botAI->IsTank(bot) && !botAI->IsRanged(bot);
}

bool LoathebTrigger::IsActive() { return helper.UpdateBossAI(); }

bool ThaddiusPhasePetTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhasePet() && helper.IsPetPhaseEngaged();
}

bool ThaddiusPhaseTransitionTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhaseTransition();
}

bool ThaddiusPhaseThaddiusTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhaseThaddius();
}
