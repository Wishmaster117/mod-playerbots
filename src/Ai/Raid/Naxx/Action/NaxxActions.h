/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NAXXACTIONS_H
#define PLAYERBOTS_NAXXACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "GenericActions.h"
#include "MovementActions.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "NaxxBossHelper.h"

class GrobbulusInjectionPositionAction : public MovementAction
{
public:
    GrobbulusInjectionPositionAction(PlayerbotAI* ai, std::string const& name) : MovementAction(ai, name) {}

protected:
    bool MoveToSafeDropPosition();
};

class GrobbulusGoBehindAction : public GrobbulusInjectionPositionAction
{
public:
    GrobbulusGoBehindAction(PlayerbotAI* ai, float /*distance*/ = 24.0f, float /*deltaAngle*/ = M_PI / 8)
        : GrobbulusInjectionPositionAction(ai, "grobbulus go behind")
    {
    }

    bool Execute(Event event) override;
};

class GrobbulusRotateAction : public RotateAroundTheCenterPointAction
{
public:
    GrobbulusRotateAction(PlayerbotAI* botAI)
        : RotateAroundTheCenterPointAction(botAI, "rotate grobbulus", 3281.23f, -3310.38f, 35.0f, 8, true, M_PI)
    {
    }

    bool isUseful() override
    {
        return RotateAroundTheCenterPointAction::isUseful() && botAI->IsMainTank(bot) &&
               AI_VALUE2(bool, "has aggro", "boss target");
    }

    uint32 GetCurrWaypoint() override;
};

class GrobbulusMoveAwayAction : public GrobbulusInjectionPositionAction
{
public:
    GrobbulusMoveAwayAction(PlayerbotAI* ai, float /*distance*/ = 18.0f)
        : GrobbulusInjectionPositionAction(ai, "grobbulus move away")
    {
    }

    bool Execute(Event event) override;
};

class GrobbulusPositionAction : public MovementAction
{
public:
    GrobbulusPositionAction(PlayerbotAI* ai) : MovementAction(ai, "grobbulus position") {}

    bool Execute(Event event) override;
};

class GrobbulusChooseTargetAction : public AttackAction
{
public:
    GrobbulusChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "grobbulus choose target") {}

    bool Execute(Event event) override;
};

class HeiganDanceAction : public MovementAction
{
public:
    HeiganDanceAction(PlayerbotAI* ai) : MovementAction(ai, "heigan dance")
    {
        initialized = false;
        platformPhase = false;
        phaseStartMs = 0;
        processedEruptions = 0;
        currentSafeSection = 3;
        direction = -1;

        platformZ = 276.54f;
        arenaZ = 264.00f;

        // The indices intentionally match the eruption section numbers used by AzerothCore.
        waypoints.push_back(std::make_pair(2755.99f, -3703.96f));  // section 0
        waypoints.push_back(std::make_pair(2762.30f, -3684.59f));  // section 1
        waypoints.push_back(std::make_pair(2775.49f, -3674.43f));  // section 2
        waypoints.push_back(std::make_pair(2794.88f, -3668.12f));  // section 3
        platform = std::make_pair(2794.26f, -3706.67f);
    }

protected:
    bool UpdateDanceState();
    bool IsPlatformPhase(Unit* boss) const;
    void ResetPhase(bool nextPlatformPhase, uint32 now);
    void AdvanceSafeSection();

    bool initialized;
    bool platformPhase;
    uint32 phaseStartMs;
    uint32 processedEruptions;
    uint8 currentSafeSection;
    int32 direction;
    float platformZ;
    float arenaZ;
    std::vector<std::pair<float, float>> waypoints;
    std::pair<float, float> platform;
};

class HeiganDanceMeleeAction : public HeiganDanceAction
{
public:
    HeiganDanceMeleeAction(PlayerbotAI* ai) : HeiganDanceAction(ai) {}
    bool Execute(Event event) override;
};

class HeiganDanceRangedAction : public HeiganDanceAction
{
public:
    HeiganDanceRangedAction(PlayerbotAI* ai) : HeiganDanceAction(ai) {}
    bool Execute(Event event) override;
};

class HeiganDispelDecrepitFeverAction : public Action
{
public:
    HeiganDispelDecrepitFeverAction(PlayerbotAI* ai) : Action(ai, "heigan dispel decrepit fever") {}

    bool Execute(Event event) override;
    bool isUseful() override;

private:
    Unit* FindTarget() const;
    bool IsDiseaseDispeller() const;
};

class FaerlinaSacrificeWorshipperAction : public AttackAction
{
public:
    FaerlinaSacrificeWorshipperAction(PlayerbotAI* ai) : AttackAction(ai, "faerlina sacrifice worshipper") {}

    bool Execute(Event event) override;
    bool isUseful() override;

private:
    Unit* FindTarget();
};

class ThaddiusAttackNearestPetAction : public AttackAction
{
public:
    ThaddiusAttackNearestPetAction(PlayerbotAI* ai) : AttackAction(ai, "thaddius attack nearest pet"), helper(ai) {}
    virtual bool Execute(Event event);
    virtual bool isUseful();

private:
    ThaddiusBossHelper helper;
};

class ThaddiusMoveToPlatformAction : public MovementAction
{
public:
    ThaddiusMoveToPlatformAction(PlayerbotAI* ai) : MovementAction(ai, "thaddius move to platform") {}
    virtual bool Execute(Event event);
    virtual bool isUseful();
};

class ThaddiusMovePolarityAction : public MovementAction
{
public:
    ThaddiusMovePolarityAction(PlayerbotAI* ai) : MovementAction(ai, "thaddius move polarity") {}
    virtual bool Execute(Event event);
    virtual bool isUseful();
};

class RazuviousUseObedienceCrystalAction : public MovementAction
{
public:
    RazuviousUseObedienceCrystalAction(PlayerbotAI* ai)
        : MovementAction(ai, "razuvious use obedience crystal"), helper(ai)
    {
    }
    bool Execute(Event event) override;

private:
    RazuviousBossHelper helper;
};

class RazuviousTargetAction : public AttackAction
{
public:
    RazuviousTargetAction(PlayerbotAI* ai) : AttackAction(ai, "razuvious target"), helper(ai) {}
    bool Execute(Event event) override;

private:
    RazuviousBossHelper helper;
};

class FourHorsemenAttractAlternativelyAction : public AttackAction
{
public:
    FourHorsemenAttractAlternativelyAction(PlayerbotAI* ai) : AttackAction(ai, "four horsemen attract alternatively"), helper(ai)
    {
    }
    bool Execute(Event event) override;

protected:
    FourHorsemenBossHelper helper;
};

class FourHorsemenAttackInOrderAction : public AttackAction
{
public:
    FourHorsemenAttackInOrderAction(PlayerbotAI* ai) : AttackAction(ai, "four horsemen attack in order"), helper(ai) {}
    bool Execute(Event event) override;

protected:
    FourHorsemenBossHelper helper;
};

// class SapphironGroundMainTankPositionAction : public MovementAction
// {
// public:
//     SapphironGroundMainTankPositionAction(PlayerbotAI* ai) : MovementAction(ai, "sapphiron ground main tank
//     position") {} virtual bool Execute(Event event);
// };

class SapphironGroundPositionAction : public MovementAction
{
public:
    SapphironGroundPositionAction(PlayerbotAI* ai) : MovementAction(ai, "sapphiron ground position"), helper(ai) {}
    bool Execute(Event event) override;

protected:
    SapphironBossHelper helper;
};

class SapphironFlightPositionAction : public MovementAction
{
public:
    SapphironFlightPositionAction(PlayerbotAI* ai) : MovementAction(ai, "sapphiron flight position"), helper(ai) {}
    bool Execute(Event event) override;

protected:
    SapphironBossHelper helper;
    bool MoveToNearestIcebolt();
};

// class SapphironAvoidChillAction : public MovementAction
// {
// public:
//     SapphironAvoidChillAction(PlayerbotAI* ai) : MovementAction(ai, "sapphiron avoid chill") {}
//     virtual bool Execute(Event event);
// };

class KelthuzadChooseTargetAction : public AttackAction
{
public:
    KelthuzadChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "kel'thuzad choose target"), helper(ai) {}
    virtual bool Execute(Event event);

private:
    KelthuzadBossHelper helper;
};

class KelthuzadPositionAction : public MovementAction
{
public:
    KelthuzadPositionAction(PlayerbotAI* ai) : MovementAction(ai, "kel'thuzad position"), helper(ai) {}
    virtual bool Execute(Event event);

private:
    KelthuzadBossHelper helper;
};

class AnubrekhanChooseTargetAction : public AttackAction
{
public:
    AnubrekhanChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "anub'rekhan choose target") {}
    bool Execute(Event event) override;
};

class AnubrekhanPositionAction : public RotateAroundTheCenterPointAction
{
public:
    AnubrekhanPositionAction(PlayerbotAI* ai)
        : RotateAroundTheCenterPointAction(ai, "anub'rekhan position", 3272.49f, -3476.27f, 45.0f, 16) {}
    bool Execute(Event event) override;
};

class GluthChooseTargetAction : public AttackAction
{
public:
    GluthChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "gluth choose target"), helper(ai) {}
    bool Execute(Event event) override;

private:
    GluthBossHelper helper;
};

class GluthPositionAction : public RotateAroundTheCenterPointAction
{
public:
    GluthPositionAction(PlayerbotAI* ai)
        : RotateAroundTheCenterPointAction(ai, "gluth position", 3293.61f, -3149.01f, 12.0f, 12), helper(ai) {}
    bool Execute(Event event) override;

private:
    GluthBossHelper helper;
};

class GluthSlowdownAction : public Action
{
public:
    GluthSlowdownAction(PlayerbotAI* ai) : Action(ai, "gluth slowdown"), helper(ai) {}
    bool Execute(Event event) override;

private:
    GluthBossHelper helper;
};

class LoathebPositionAction : public MovementAction
{
public:
    LoathebPositionAction(PlayerbotAI* ai) : MovementAction(ai, "loatheb position"), helper(ai) {}
    virtual bool Execute(Event event);

private:
    LoathebBossHelper helper;
};

class LoathebChooseTargetAction : public AttackAction
{
public:
    LoathebChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "loatheb choose target"), helper(ai) {}
    virtual bool Execute(Event event);

private:
    LoathebBossHelper helper;
};

class GothikMoveToAssignedSideAction : public MovementAction
{
public:
    GothikMoveToAssignedSideAction(PlayerbotAI* ai) : MovementAction(ai, "gothik move to assigned side") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class GothikChooseTargetAction : public AttackAction
{
public:
    GothikChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "gothik choose target") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class NothChooseTargetAction : public AttackAction
{
public:
    NothChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "noth choose target"), helper(ai) {}
    bool Execute(Event event) override;

private:
    NothBossHelper helper;
};

class NothPositionAction : public MovementAction
{
public:
    NothPositionAction(PlayerbotAI* ai) : MovementAction(ai, "noth position"), helper(ai) {}
    bool Execute(Event event) override;

private:
    NothBossHelper helper;
};

class MaexxnaAttackWebWrapAction : public AttackAction
{
public:
    MaexxnaAttackWebWrapAction(PlayerbotAI* ai) : AttackAction(ai, "maexxna attack web wrap") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class MaexxnaTankSpiderlingsAction : public AttackAction
{
public:
    MaexxnaTankSpiderlingsAction(PlayerbotAI* ai) : AttackAction(ai, "maexxna tank spiderlings") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class PatchwerkRangedPositionAction : public MovementAction
{
public:
    PatchwerkRangedPositionAction(PlayerbotAI* ai) : MovementAction(ai, "patchwerk ranged position") {}
    bool Execute(Event event) override;
};

#endif
