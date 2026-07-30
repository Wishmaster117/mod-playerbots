/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NAXXTRIGGERS_H
#define PLAYERBOTS_NAXXTRIGGERS_H

#include "EventMap.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "NaxxBossHelper.h"
#include "Trigger.h"

class MutatingInjectionTrigger : public Trigger
{
public:
    MutatingInjectionTrigger(PlayerbotAI* ai) : Trigger(ai, "mutating injection", 1) {}
    bool IsActive() override;
};

class MutatingInjectionMeleeTrigger : public MutatingInjectionTrigger
{
public:
    MutatingInjectionMeleeTrigger(PlayerbotAI* ai) : MutatingInjectionTrigger(ai) {}
    bool IsActive() override;
};

class MutatingInjectionRangedTrigger : public MutatingInjectionTrigger
{
public:
    MutatingInjectionRangedTrigger(PlayerbotAI* ai) : MutatingInjectionTrigger(ai) {}
    bool IsActive() override;
};

class AuraRemovedTrigger : public Trigger
{
public:
    AuraRemovedTrigger(PlayerbotAI* botAI, std::string name) : Trigger(botAI, name, 1)
    {
        this->prev_check = false;
    }
    bool IsActive() override;

protected:
    bool prev_check;
};

class MutatingInjectionRemovedTrigger : public Trigger
{
public:
    MutatingInjectionRemovedTrigger(PlayerbotAI* ai) : Trigger(ai, "mutating injection removed", 1), hadInjection(false) {}
    bool IsActive() override;

private:
    bool hadInjection;
};

class GrobbulusCloudTrigger : public Trigger
{
public:
    GrobbulusCloudTrigger(PlayerbotAI* ai)
        : Trigger(ai, "grobbulus cloud event"), combatStarted(false), nextRotationMs(0), lastRotationMs(0), lastCloudGuid(0)
    {
    }

    bool IsActive() override;

private:
    bool combatStarted;
    uint32 nextRotationMs;
    uint32 lastRotationMs;
    uint64 lastCloudGuid;
    static constexpr uint32 CloudRotationPeriodMs = 15000;
    static constexpr uint32 CloudFallbackDelayMs = 16000;
};

class GrobbulusTrigger : public Trigger
{
public:
    GrobbulusTrigger(PlayerbotAI* ai) : Trigger(ai, "grobbulus") {}
    bool IsActive() override;
};

class HeiganMeleeTrigger : public Trigger
{
public:
    HeiganMeleeTrigger(PlayerbotAI* ai) : Trigger(ai, "heigan melee") {}
    bool IsActive() override;
};

class HeiganRangedTrigger : public Trigger
{
public:
    HeiganRangedTrigger(PlayerbotAI* ai) : Trigger(ai, "heigan ranged") {}
    bool IsActive() override;
};

class HeiganDecrepitFeverTrigger : public Trigger
{
public:
    HeiganDecrepitFeverTrigger(PlayerbotAI* ai) : Trigger(ai, "heigan decrepit fever") {}
    bool IsActive() override;
};

class RazuviousTankTrigger : public Trigger
{
public:
    RazuviousTankTrigger(PlayerbotAI* ai) : Trigger(ai, "instructor razuvious tank"), helper(ai) {}
    bool IsActive() override;

private:
    RazuviousBossHelper helper;
};

class RazuviousNontankTrigger : public Trigger
{
public:
    RazuviousNontankTrigger(PlayerbotAI* ai) : Trigger(ai, "instructor razuvious non-tank"), helper(ai) {}
    bool IsActive() override;

private:
    RazuviousBossHelper helper;
};

class KelthuzadTrigger : public Trigger
{
public:
    KelthuzadTrigger(PlayerbotAI* ai) : Trigger(ai, "kel'thuzad trigger"), helper(ai) {}
    bool IsActive() override;

private:
    KelthuzadBossHelper helper;
};

class AnubrekhanTrigger : public Trigger
{
public:
    AnubrekhanTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'rekhan") {}
    bool IsActive() override;
};

class FaerlinaTrigger : public Trigger
{
public:
    FaerlinaTrigger(PlayerbotAI* ai) : Trigger(ai, "faerlina") {}
    bool IsActive() override;
};

class FaerlinaFrenzyTrigger : public Trigger
{
public:
    FaerlinaFrenzyTrigger(PlayerbotAI* ai) : Trigger(ai, "faerlina frenzy") {}
    bool IsActive() override;
};

class MaexxnaTrigger : public Trigger
{
public:
    MaexxnaTrigger(PlayerbotAI* ai) : Trigger(ai, "maexxna") {}
    bool IsActive() override;
};

class MaexxnaWebWrapTrigger : public Trigger
{
public:
    MaexxnaWebWrapTrigger(PlayerbotAI* ai) : Trigger(ai, "maexxna web wrap") {}
    bool IsActive() override;
};

class MaexxnaSpiderlingsTrigger : public Trigger
{
public:
    MaexxnaSpiderlingsTrigger(PlayerbotAI* ai) : Trigger(ai, "maexxna spiderlings") {}
    bool IsActive() override;
};

class GothikMoveToAssignedSideTrigger : public Trigger
{
public:
    GothikMoveToAssignedSideTrigger(PlayerbotAI* ai) : Trigger(ai, "gothik move to assigned side") {}
    bool IsActive() override;
};

class GothikChooseTargetTrigger : public Trigger
{
public:
    GothikChooseTargetTrigger(PlayerbotAI* ai) : Trigger(ai, "gothik choose target") {}
    bool IsActive() override;
};

class NothTrigger : public Trigger
{
public:
    NothTrigger(PlayerbotAI* ai) : Trigger(ai, "noth"), helper(ai) {}
    bool IsActive() override;

private:
    NothBossHelper helper;
};

class NothCurseTrigger : public Trigger
{
public:
    NothCurseTrigger(PlayerbotAI* ai) : Trigger(ai, "noth curse"), helper(ai) {}
    bool IsActive() override;

private:
    NothBossHelper helper;
};

class PatchwerkTankTrigger : public Trigger
{
public:
    PatchwerkTankTrigger(PlayerbotAI* ai) : Trigger(ai, "patchwerk tank") {}
    bool IsActive() override;
};

class PatchwerkNonTankTrigger : public Trigger
{
public:
    PatchwerkNonTankTrigger(PlayerbotAI* ai) : Trigger(ai, "patchwerk non-tank") {}
    bool IsActive() override;
};

class PatchwerkRangedTrigger : public Trigger
{
public:
    PatchwerkRangedTrigger(PlayerbotAI* ai) : Trigger(ai, "patchwerk ranged") {}
    bool IsActive() override;
};

class ThaddiusPhasePetTrigger : public Trigger
{
public:
    ThaddiusPhasePetTrigger(PlayerbotAI* ai) : Trigger(ai, "thaddius phase pet"), helper(ai) {}
    bool IsActive() override;

private:
    ThaddiusBossHelper helper;
};

class ThaddiusPhasePetLoseAggroTrigger : public ThaddiusPhasePetTrigger
{
public:
    ThaddiusPhasePetLoseAggroTrigger(PlayerbotAI* ai) : ThaddiusPhasePetTrigger(ai) {}
    virtual bool IsActive()
    {
        Unit* target = AI_VALUE(Unit*, "current target");
        return ThaddiusPhasePetTrigger::IsActive() && botAI->IsTank(bot) && target && target->GetVictim() != bot;
    }
};

class ThaddiusPhaseTransitionTrigger : public Trigger
{
public:
    ThaddiusPhaseTransitionTrigger(PlayerbotAI* ai) : Trigger(ai, "thaddius phase transition"), helper(ai) {}
    bool IsActive() override;

private:
    ThaddiusBossHelper helper;
};

class ThaddiusPhaseThaddiusTrigger : public Trigger
{
public:
    ThaddiusPhaseThaddiusTrigger(PlayerbotAI* ai) : Trigger(ai, "thaddius phase thaddius"), helper(ai) {}
    bool IsActive() override;

private:
    ThaddiusBossHelper helper;
};

class FourHorsemenAttractorsTrigger : public Trigger
{
public:
    FourHorsemenAttractorsTrigger(PlayerbotAI* ai) : Trigger(ai, "four horsemen attractors"), helper(ai) {}
    bool IsActive() override;

private:
    FourHorsemenBossHelper helper;
};

class FourHorsemenExceptAttractorsTrigger : public Trigger
{
public:
    FourHorsemenExceptAttractorsTrigger(PlayerbotAI* ai) : Trigger(ai, "four horsemen except attractors"), helper(ai) {}
    bool IsActive() override;

private:
    FourHorsemenBossHelper helper;
};

class SapphironGroundTrigger : public Trigger
{
public:
    SapphironGroundTrigger(PlayerbotAI* ai) : Trigger(ai, "sapphiron ground"), helper(ai) {}
    bool IsActive() override;

private:
    SapphironBossHelper helper;
};

class SapphironFlightTrigger : public Trigger
{
public:
    SapphironFlightTrigger(PlayerbotAI* ai) : Trigger(ai, "sapphiron flight"), helper(ai) {}
    bool IsActive() override;

private:
    SapphironBossHelper helper;
};

class GluthTrigger : public Trigger
{
public:
    GluthTrigger(PlayerbotAI* ai) : Trigger(ai, "gluth trigger"), helper(ai) {}
    bool IsActive() override;

private:
    GluthBossHelper helper;
};

class GluthMainTankMortalWoundTrigger : public Trigger
{
public:
    GluthMainTankMortalWoundTrigger(PlayerbotAI* ai) : Trigger(ai, "gluth main tank mortal wound trigger"), helper(ai) {}
    bool IsActive() override;

private:
    GluthBossHelper helper;
};

class LoathebTrigger : public Trigger
{
public:
    LoathebTrigger(PlayerbotAI* ai) : Trigger(ai, "loatheb"), helper(ai) {}
    bool IsActive() override;

private:
    LoathebBossHelper helper;
};

#endif
