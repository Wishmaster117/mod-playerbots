/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NAXXACTIONCONTEXT_H
#define PLAYERBOTS_NAXXACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "NaxxActions.h"

class RaidNaxxActionContext : public NamedObjectContext<Action>
{
public:
    RaidNaxxActionContext()
    {
        creators["grobbulus go behind the boss"] = &RaidNaxxActionContext::go_behind_the_boss;
        creators["rotate grobbulus"] = &RaidNaxxActionContext::rotate_grobbulus;
        creators["grobbulus move away"] = &RaidNaxxActionContext::grobbulus_move_away;
        creators["grobbulus position"] = &RaidNaxxActionContext::grobbulus_position;
        creators["grobbulus choose target"] = &RaidNaxxActionContext::grobbulus_choose_target;

        creators["heigan dance melee"] = &RaidNaxxActionContext::heigan_dance_melee;
        creators["heigan dance ranged"] = &RaidNaxxActionContext::heigan_dance_ranged;
        creators["heigan dispel decrepit fever"] = &RaidNaxxActionContext::heigan_dispel_decrepit_fever;
        creators["thaddius attack nearest pet"] = &RaidNaxxActionContext::thaddius_attack_nearest_pet;
        creators["thaddius move to platform"] = &RaidNaxxActionContext::thaddius_move_to_platform;
        creators["thaddius move polarity"] = &RaidNaxxActionContext::thaddius_move_polarity;

        creators["razuvious use obedience crystal"] = &RaidNaxxActionContext::razuvious_use_obedience_crystal;
        creators["razuvious target"] = &RaidNaxxActionContext::razuvious_target;

        creators["four horsemen attract alternatively"] = &RaidNaxxActionContext::four_horsemen_attract_alternatively;
        creators["four horsemen attack in order"] = &RaidNaxxActionContext::four_horsemen_attack_in_order;

        creators["sapphiron ground position"] = &RaidNaxxActionContext::sapphiron_ground_position;
        creators["sapphiron flight position"] = &RaidNaxxActionContext::sapphiron_flight_position;

        creators["kel'thuzad choose target"] = &RaidNaxxActionContext::kelthuzad_choose_target;
        creators["kel'thuzad position"] = &RaidNaxxActionContext::kelthuzad_position;

        creators["anub'rekhan choose target"] = &RaidNaxxActionContext::anubrekhan_choose_target;
        creators["anub'rekhan position"] = &RaidNaxxActionContext::anubrekhan_position;

        creators["faerlina sacrifice worshipper"] = &RaidNaxxActionContext::faerlina_sacrifice_worshipper;
        creators["maexxna attack web wrap"] = &RaidNaxxActionContext::maexxna_attack_web_wrap;
        creators["maexxna tank spiderlings"] = &RaidNaxxActionContext::maexxna_tank_spiderlings;

        creators["gothik move to assigned side"] = &RaidNaxxActionContext::gothik_move_to_assigned_side;
        creators["gothik choose target"] = &RaidNaxxActionContext::gothik_choose_target;

        creators["noth position"] = &RaidNaxxActionContext::noth_position;
        creators["noth choose target"] = &RaidNaxxActionContext::noth_choose_target;

        creators["gluth choose target"] = &RaidNaxxActionContext::gluth_choose_target;
        creators["gluth position"] = &RaidNaxxActionContext::gluth_position;
        creators["gluth slowdown"] = &RaidNaxxActionContext::gluth_slowdown;

        creators["patchwerk ranged position"] = &RaidNaxxActionContext::patchwerk_ranged_position;

        creators["loatheb position"] = &RaidNaxxActionContext::loatheb_position;
        creators["loatheb choose target"] = &RaidNaxxActionContext::loatheb_choose_target;
    }

private:
    static Action* go_behind_the_boss(PlayerbotAI* ai) { return new GrobbulusGoBehindAction(ai); }
    static Action* rotate_grobbulus(PlayerbotAI* ai) { return new GrobbulusRotateAction(ai); }
    static Action* grobbulus_move_away(PlayerbotAI* ai) { return new GrobbulusMoveAwayAction(ai); }
    static Action* grobbulus_position(PlayerbotAI* ai) { return new GrobbulusPositionAction(ai); }
    static Action* grobbulus_choose_target(PlayerbotAI* ai) { return new GrobbulusChooseTargetAction(ai); }
    static Action* heigan_dance_melee(PlayerbotAI* ai) { return new HeiganDanceMeleeAction(ai); }
    static Action* heigan_dance_ranged(PlayerbotAI* ai) { return new HeiganDanceRangedAction(ai); }
    static Action* heigan_dispel_decrepit_fever(PlayerbotAI* ai) { return new HeiganDispelDecrepitFeverAction(ai); }
    static Action* thaddius_attack_nearest_pet(PlayerbotAI* ai) { return new ThaddiusAttackNearestPetAction(ai); }
    static Action* thaddius_move_to_platform(PlayerbotAI* ai) { return new ThaddiusMoveToPlatformAction(ai); }
    static Action* thaddius_move_polarity(PlayerbotAI* ai) { return new ThaddiusMovePolarityAction(ai); }
    static Action* razuvious_target(PlayerbotAI* ai) { return new RazuviousTargetAction(ai); }
    static Action* razuvious_use_obedience_crystal(PlayerbotAI* ai)
    {
        return new RazuviousUseObedienceCrystalAction(ai);
    }
    static Action* four_horsemen_attract_alternatively(PlayerbotAI* ai) { return new FourHorsemenAttractAlternativelyAction(ai); }
    static Action* four_horsemen_attack_in_order(PlayerbotAI* ai) { return new FourHorsemenAttackInOrderAction(ai); }
    static Action* sapphiron_ground_position(PlayerbotAI* ai) { return new SapphironGroundPositionAction(ai); }
    static Action* sapphiron_flight_position(PlayerbotAI* ai) { return new SapphironFlightPositionAction(ai); }
    static Action* kelthuzad_choose_target(PlayerbotAI* ai) { return new KelthuzadChooseTargetAction(ai); }
    static Action* kelthuzad_position(PlayerbotAI* ai) { return new KelthuzadPositionAction(ai); }
    static Action* anubrekhan_choose_target(PlayerbotAI* ai) { return new AnubrekhanChooseTargetAction(ai); }
    static Action* anubrekhan_position(PlayerbotAI* ai) { return new AnubrekhanPositionAction(ai); }
    static Action* faerlina_sacrifice_worshipper(PlayerbotAI* ai) { return new FaerlinaSacrificeWorshipperAction(ai); }
    static Action* maexxna_attack_web_wrap(PlayerbotAI* ai) { return new MaexxnaAttackWebWrapAction(ai); }
    static Action* maexxna_tank_spiderlings(PlayerbotAI* ai) { return new MaexxnaTankSpiderlingsAction(ai); }
    static Action* gothik_move_to_assigned_side(PlayerbotAI* ai) { return new GothikMoveToAssignedSideAction(ai); }
    static Action* gothik_choose_target(PlayerbotAI* ai) { return new GothikChooseTargetAction(ai); }
    static Action* noth_position(PlayerbotAI* ai) { return new NothPositionAction(ai); }
    static Action* noth_choose_target(PlayerbotAI* ai) { return new NothChooseTargetAction(ai); }
    static Action* gluth_choose_target(PlayerbotAI* ai) { return new GluthChooseTargetAction(ai); }
    static Action* gluth_position(PlayerbotAI* ai) { return new GluthPositionAction(ai); }
    static Action* gluth_slowdown(PlayerbotAI* ai) { return new GluthSlowdownAction(ai); }
    static Action* patchwerk_ranged_position(PlayerbotAI* ai) { return new PatchwerkRangedPositionAction(ai); }
    static Action* loatheb_position(PlayerbotAI* ai) { return new LoathebPositionAction(ai); }
    static Action* loatheb_choose_target(PlayerbotAI* ai) { return new LoathebChooseTargetAction(ai); }
};

#endif
