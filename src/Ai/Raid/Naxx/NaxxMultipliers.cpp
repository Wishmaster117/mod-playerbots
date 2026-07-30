/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxMultipliers.h"

#include "ChooseTargetActions.h"
#include "DKActions.h"
#include "DruidActions.h"
#include "DruidBearActions.h"
#include "FollowActions.h"
#include "GenericActions.h"
#include "GenericSpellActions.h"
#include "HunterActions.h"
#include "MageActions.h"
#include "MovementActions.h"
#include "PaladinActions.h"
#include "PriestActions.h"
#include "NaxxActions.h"
#include "NaxxSpellIds.h"
#include "ReachTargetActions.h"
#include "RogueActions.h"
#include "ScriptedCreature.h"
#include "ShamanActions.h"
#include "Spell.h"
#include "Timer.h"
#include "UseMeetingStoneAction.h"
#include "WarriorActions.h"

float GrobbulusMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss || !boss->IsAlive())
        return 1.0f;

    bool const injected = NaxxSpellIds::HasAnyAura(bot, {NaxxSpellIds::MutatingInjection}) ||
                          botAI->HasAura("mutating injection", bot, false, false, -1, true);

    if (injected)
    {
        if (dynamic_cast<GrobbulusInjectionPositionAction*>(action))
            return 10.0f;

        if (dynamic_cast<MovementAction*>(action) || dynamic_cast<MeleeAction*>(action) ||
            (dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastHealingSpellAction*>(action)))
        {
            return 0.0f;
        }
    }

    if (dynamic_cast<GrobbulusRotateAction*>(action))
        return botAI->IsMainTank(bot) ? 8.0f : 0.0f;

    if (dynamic_cast<GrobbulusPositionAction*>(action))
        return injected ? 0.0f : 3.0f;

    if (dynamic_cast<GrobbulusChooseTargetAction*>(action))
        return 2.0f;

    if (dynamic_cast<AvoidAoeAction*>(action))
        return botAI->IsMainTank(bot) ? 0.0f : 1.0f;

    if (dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FollowAction*>(action) ||
        dynamic_cast<FleeAction*>(action) || dynamic_cast<CastDisengageAction*>(action) ||
        dynamic_cast<CastBlinkBackAction*>(action))
    {
        return 0.0f;
    }

    return 1.0f;
}

float HeiganDanceMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!boss || !boss->IsAlive())
        return 1.0f;

    if (!bot->IsInCombat() && !boss->IsInCombat())
        return 1.0f;

    bool nextPlatformPhase = boss->GetPositionZ() >= 270.0f &&
                             boss->IsWithinDist2d(2794.26f, -3706.67f, 14.0f);
    uint32 now = getMSTime();
    if (!initialized || nextPlatformPhase != platformPhase)
    {
        initialized = true;
        platformPhase = nextPlatformPhase;
        phaseStartMs = now;
    }

    if (dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FollowAction*>(action) ||
        dynamic_cast<FleeAction*>(action) || dynamic_cast<CastDisengageAction*>(action) ||
        dynamic_cast<CastBlinkBackAction*>(action))
    {
        return 0.0f;
    }

    if (dynamic_cast<CastAspectOfThePackAction*>(action))
        return platformPhase ? 1.0f : 0.0f;

    if (dynamic_cast<HeiganDanceAction*>(action))
        return 3.0f;

    if (dynamic_cast<HeiganDispelDecrepitFeverAction*>(action))
        return platformPhase ? 0.0f : 4.0f;

    uint32 elapsed = now - phaseStartMs;
    bool movementWindow = false;
    if (platformPhase)
    {
        // The fast dance changes section every four seconds. Keep movement dominant for the full phase.
        movementWindow = true;
    }
    else if (elapsed < 3000)
    {
        // Establish section 3 immediately at the start of each slow phase.
        movementWindow = true;
    }
    else if (elapsed >= 15250)
    {
        uint32 sinceMovement = (elapsed - 15250) % 10000;
        movementWindow = sinceMovement < 2500;
    }

    if (!movementWindow)
        return 1.0f;

    if (dynamic_cast<MovementAction*>(action))
        return 0.0f;

    if (dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastMeleeSpellAction*>(action))
    {
        CastSpellAction* spellAction = dynamic_cast<CastSpellAction*>(action);
        uint32 spellId = AI_VALUE2(uint32, "spell id", spellAction->getSpell());
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (spellInfo && spellInfo->CalcCastTime() == 0 && !spellInfo->IsChanneled())
            return 1.0f;
    }

    return 0.0f;
}

float LoathebGenericMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "loatheb");
    if (!boss)
        return 1.0f;

    if (dynamic_cast<LoathebChooseTargetAction*>(action))
        return 4.0f;

    if (dynamic_cast<LoathebPositionAction*>(action))
        return 3.0f;

    context->GetValue<bool>("neglect threat")->Set(true);
    if (botAI->GetState() == BOT_STATE_COMBAT &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) || dynamic_cast<FleeAction*>(action) ||
         dynamic_cast<FollowAction*>(action) || dynamic_cast<CombatFormationMoveAction*>(action)))
    {
        return 0.0f;
    }

    if (!dynamic_cast<CastHealingSpellAction*>(action))
        return 1.0f;

    Aura* aura = NaxxSpellIds::GetAnyAura(bot, {NaxxSpellIds::NecroticAura10});
    if (!aura)
    {
        // Fallback to name for custom spell data.
        aura = botAI->GetAura("necrotic aura", bot);
    }

    // Healing is only effective during the short gap before Necrotic Aura is reapplied.
    if (!aura || aura->GetDuration() <= 1500)
        return 4.0f;

    return 0.0f;
}

float ThaddiusGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (helper.IsPhasePet())
    {
        if (dynamic_cast<ThaddiusAttackNearestPetAction*>(action))
            return 5.0f;

        if (dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FollowAction*>(action) ||
            dynamic_cast<FleeAction*>(action) || dynamic_cast<DpsAssistAction*>(action) ||
            dynamic_cast<TankAssistAction*>(action) || dynamic_cast<CastDebuffSpellOnAttackerAction*>(action))
        {
            return 0.0f;
        }

        if (!botAI->IsTank(bot) && dynamic_cast<ReachSpellAction*>(action))
            return 0.0f;

        if (dynamic_cast<ReachPartyMemberToHealAction*>(action) || dynamic_cast<BuffOnMainTankAction*>(action))
            return 0.0f;

        Unit* target = AI_VALUE(Unit*, "current target");
        Unit* feugen = helper.GetFeugen();
        Unit* stalagg = helper.GetStalagg();
        if (target && feugen && stalagg && (target == feugen || target == stalagg) && feugen->IsAlive() && stalagg->IsAlive())
        {
            Unit* other = target == feugen ? stalagg : feugen;
            float const targetHealth = target->GetHealthPct();
            float const otherHealth = other->GetHealthPct();
            bool const hardHold = targetHealth <= 12.0f && otherHealth > 12.0f;
            bool const softHold = targetHealth <= 30.0f && otherHealth - targetHealth >= 4.0f;

            if ((hardHold || softHold) && !botAI->IsTank(bot))
            {
                if (dynamic_cast<MeleeAction*>(action))
                    return 0.0f;
                if (dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastHealingSpellAction*>(action))
                    return 0.0f;
            }
        }
    }

    if (helper.IsPhaseTransition())
    {
        if (dynamic_cast<ThaddiusMoveToPlatformAction*>(action))
            return 5.0f;
        if (dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FollowAction*>(action) || dynamic_cast<FleeAction*>(action))
            return 0.0f;
    }

    if (helper.IsPhaseThaddius())
    {
        if (dynamic_cast<ThaddiusMovePolarityAction*>(action))
            return 5.0f;
        if (dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FollowAction*>(action) || dynamic_cast<FleeAction*>(action))
            return 0.0f;
    }

    return 1.0f;
}

float SapphironGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (dynamic_cast<CastDeathGripAction*>(action) || dynamic_cast<CombatFormationMoveAction*>(action) ||
        dynamic_cast<FollowAction*>(action) || dynamic_cast<FleeAction*>(action) || dynamic_cast<AvoidAoeAction*>(action))
    {
        return 0.0f;
    }

    if (helper.IsPhaseGround() && dynamic_cast<SapphironGroundPositionAction*>(action))
        return 4.0f;

    if (helper.IsPhaseFlight() && dynamic_cast<SapphironFlightPositionAction*>(action))
        return helper.WaitForExplosion() ? 10.0f : 6.0f;

    if (helper.HasLifeDrainInGroup())
    {
        if (dynamic_cast<CurePartyMemberAction*>(action))
            return 7.0f;

        if (botAI->IsHeal(bot))
        {
            if (dynamic_cast<CastHealingSpellAction*>(action) || dynamic_cast<HealPartyMemberAction*>(action) ||
                dynamic_cast<CastAoeHealSpellAction*>(action))
            {
                return 3.0f;
            }
        }
    }

    if (helper.IsPhaseFlight() && helper.WaitForExplosion())
    {
        if (dynamic_cast<MovementAction*>(action))
            return 0.0f;

        if (botAI->IsHeal(bot) &&
            (dynamic_cast<CastHealingSpellAction*>(action) || dynamic_cast<HealPartyMemberAction*>(action) ||
             dynamic_cast<CastAoeHealSpellAction*>(action) || dynamic_cast<CurePartyMemberAction*>(action)))
        {
            return 2.0f;
        }

        if (dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastHealingSpellAction*>(action))
            return 0.0f;
    }

    return 1.0f;
}

float InstructorRazuviousGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    context->GetValue<bool>("neglect threat")->Set(true);

    if (dynamic_cast<RazuviousUseObedienceCrystalAction*>(action))
        return 5.0f;
    if (dynamic_cast<RazuviousTargetAction*>(action))
        return 3.0f;

    if (botAI->GetState() == BOT_STATE_COMBAT &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastTauntAction*>(action) || dynamic_cast<CastDarkCommandAction*>(action) ||
         dynamic_cast<CastHandOfReckoningAction*>(action) || dynamic_cast<CastGrowlAction*>(action)))
    {
        return 0.0f;
    }

    return 1.0f;
}

float KelthuzadGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (dynamic_cast<KelthuzadPositionAction*>(action))
    {
        if (helper.HasChains(bot) || helper.HasDetonateMana(bot))
            return 10.0f;
        if (helper.HasAuraInGroup(NaxxSpellIds::FrostBlast) || helper.GetAnyShadowFissure())
            return 8.0f;
        return 5.0f;
    }

    if (dynamic_cast<KelthuzadChooseTargetAction*>(action))
        return 4.0f;

    if (helper.HasChains(bot))
        return 0.0f;

    if (helper.HasDetonateMana(bot))
        return 0.0f;

    if (botAI->IsHeal(bot) && helper.HasAuraInGroup(NaxxSpellIds::FrostBlast))
    {
        if (dynamic_cast<CastHealingSpellAction*>(action) || dynamic_cast<HealPartyMemberAction*>(action) ||
            dynamic_cast<CastAoeHealSpellAction*>(action))
        {
            return 8.0f;
        }
        return 0.0f;
    }

    if (helper.IsPhaseTwo() && helper.IsBossCasting(NaxxSpellIds::FrostBoltSingle))
    {
        std::string const& name = action->getName();
        if (name == "kick" || name == "pummel" || name == "shield bash" || name == "mind freeze" ||
            name == "strangulate" || name == "counterspell" || name == "wind shear" || name == "spell lock" ||
            name == "silencing shot" || name == "bash" || name == "hammer of justice")
        {
            return 8.0f;
        }
    }

    std::vector<Unit*> const guardians = helper.GetGuardians();
    bool const isOffTank = botAI->IsTank(bot) && !botAI->IsMainTank(bot) && botAI->IsAssistTank(bot);
    if (isOffTank && !guardians.empty() &&
        (dynamic_cast<CastTauntAction*>(action) || dynamic_cast<CastDarkCommandAction*>(action) ||
         dynamic_cast<CastHandOfReckoningAction*>(action) || dynamic_cast<CastGrowlAction*>(action)))
    {
        return 6.0f;
    }

    if (dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FollowAction*>(action) ||
        dynamic_cast<FleeAction*>(action) || dynamic_cast<DpsAssistAction*>(action) ||
        dynamic_cast<TankAssistAction*>(action) || dynamic_cast<CastDebuffSpellOnAttackerAction*>(action))
    {
        return 0.0f;
    }

    if (helper.IsPhaseOne())
    {
        if (dynamic_cast<CastTotemAction*>(action) || dynamic_cast<CastShadowfiendAction*>(action) ||
            dynamic_cast<CastRaiseDeadAction*>(action) || dynamic_cast<CastFeignDeathAction*>(action) ||
            dynamic_cast<CastInvisibilityAction*>(action) || dynamic_cast<CastVanishAction*>(action) ||
            dynamic_cast<PetAttackAction*>(action))
        {
            return 0.0f;
        }
    }

    if (dynamic_cast<PetAttackAction*>(action))
    {
        Unit* target = AI_VALUE(Unit*, "current target");
        if (!helper.IsWithinRoom(target, KelthuzadBossHelper::ROOM_MAX_RADIUS))
            return 0.0f;
    }

    if (helper.IsPhaseTwo() && (dynamic_cast<CastBlizzardAction*>(action) || dynamic_cast<CastFrostNovaAction*>(action)))
        return 0.0f;

    return 1.0f;
}

float AnubrekhanGenericMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
        return 1.0f;

    if (NaxxSpellIds::HasAnyAura(
            boss, {NaxxSpellIds::LocustSwarm10, NaxxSpellIds::LocustSwarm10Alt, NaxxSpellIds::LocustSwarm25}) ||
        botAI->HasAura("locust swarm", boss))
    {
        if (dynamic_cast<FleeAction*>(action))
            return 0.0f;
    }
    return 1.0f;
}

float FourHorsemenGenericMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "sir zeliek");
    if (!boss)
        boss = AI_VALUE2(Unit*, "find target", "lady blaumeux");
    if (!boss)
        return 1.0f;

    context->GetValue<bool>("neglect threat")->Set(true);

    if (dynamic_cast<FourHorsemenAttractAlternativelyAction*>(action))
        return 5.0f;
    if (dynamic_cast<FourHorsemenAttackInOrderAction*>(action))
        return 3.0f;

    if (dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FollowAction*>(action) || dynamic_cast<FleeAction*>(action) ||
        dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action))
    {
        return 0.0f;
    }

    return 1.0f;
}

float GothikGenericMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    if (!boss)
        return 1.0f;

    if (dynamic_cast<CombatFormationMoveAction*>(action) && bot->GetDistance(boss) <= 160.0f)
        return 0.0f;

    if ((bot->IsInCombat() || boss->IsInCombat()) &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastDebuffSpellOnAttackerAction*>(action)))
    {
        return 0.0f;
    }

    return 1.0f;
}

float NothGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (helper.HasCurseInGroup() && dynamic_cast<CurePartyMemberAction*>(action))
        return 4.0f;

    if (helper.IsBlinkWindow() && !botAI->IsTank(bot))
    {
        if (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
            dynamic_cast<CastDebuffSpellOnAttackerAction*>(action))
        {
            return 0.0f;
        }

        if (dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastHealingSpellAction*>(action))
            return 0.0f;
    }

    return 1.0f;
}

float GluthGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (dynamic_cast<GluthChooseTargetAction*>(action))
        return 4.0f;

    if (dynamic_cast<GluthPositionAction*>(action))
        return 3.0f;

    if (dynamic_cast<GluthSlowdownAction*>(action))
        return helper.IsSlowdownHunter(bot) ? 3.0f : 0.0f;

    if (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
        dynamic_cast<FleeAction*>(action) || dynamic_cast<FollowAction*>(action) ||
        dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) ||
        dynamic_cast<CastStarfallAction*>(action))
    {
        return 0.0f;
    }

    if (botAI->IsMainTank(bot))
    {
        Aura* aura = NaxxSpellIds::GetAnyAura(bot, {NaxxSpellIds::MortalWound10, NaxxSpellIds::MortalWound25});
        if (!aura)
        {
            // Fallback to name for custom spell data.
            aura = botAI->GetAura("mortal wound", bot, false, true);
        }

        if (aura && aura->GetStackAmount() >= 5 &&
            (dynamic_cast<CastTauntAction*>(action) || dynamic_cast<CastDarkCommandAction*>(action) ||
             dynamic_cast<CastHandOfReckoningAction*>(action) || dynamic_cast<CastGrowlAction*>(action)))
        {
            return 0.0f;
        }
    }

    if (dynamic_cast<PetAttackAction*>(action))
    {
        Unit* target = AI_VALUE(Unit*, "current target");
        if (helper.IsZombieChow(target))
            return 0.0f;
    }

    return 1.0f;
}
