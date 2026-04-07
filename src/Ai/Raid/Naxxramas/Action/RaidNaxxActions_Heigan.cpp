#include "RaidNaxxActions.h"

#include "Playerbots.h"
#include "RaidNaxxSpellIds.h"
#include "Spell.h"
#include "Timer.h"

bool HeiganDanceAction::CalculateSafe()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!boss)
    {
        return false;
    }
    uint32 now = getMSTime();
    platform_phase = boss->IsWithinDist2d(platform.first, platform.second, 10.0f);

    // Phase 2: boss on platform "fast dance"
    if (platform_phase)
    {
        if (!last_platform_phase)
        {
            ResetSafe();
            phase2_start_ms = now;
            phase2_last_ticks = 0;
        }

        // - firstEruptionDelayMs: time after teleport for the first eruption tick.
        // - periodMs: eruption tick interval.
        // - moveDelayMs: shift the zone-change slightly AFTER the tick so bots move after the eruption.
        static constexpr uint32 firstEruptionDelayMs = 8000;
        static constexpr uint32 periodMs = 4000;
        static constexpr uint32 moveDelayMs = 150;

        uint32 base = phase2_start_ms + firstEruptionDelayMs + moveDelayMs;
        uint32 ticks = 0;
        if (now >= base)
        {
            ticks = 1u + (now - base) / periodMs; // after 1st tick -> go zone 2, etc.
        }

        if (ticks > phase2_last_ticks)
        {
            uint32 delta = ticks - phase2_last_ticks;
            for (uint32 i = 0; i < delta; ++i)
            {
                NextSafe();
            }
            phase2_last_ticks = ticks;
        }

        last_platform_phase = true;
        return true;
    }

    last_platform_phase = false;
    if (last_eruption_ms != 0 && now - last_eruption_ms > 15000)
    {
        ResetSafe();
    }
    if (boss->HasUnitState(UNIT_STATE_CASTING))
    {
        Spell* spell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
        {
            spell = boss->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        }
        if (spell)
        {
            SpellInfo const* info = spell->GetSpellInfo();
            bool isEruption = NaxxSpellIds::MatchesAnySpellId(info, {NaxxSpellIds::Eruption10});
            if (!isEruption && info && info->SpellName[LOCALE_enUS])
            {
                // Fallback to name for custom spell data.
                isEruption = botAI->EqualLowercaseName(info->SpellName[LOCALE_enUS], "eruption");
            }
            if (isEruption)
            {
                if (last_eruption_ms == 0 || now - last_eruption_ms > 500)
                {
                    NextSafe();
                }
                last_eruption_ms = now;
            }
        }
    }
    return true;
}

bool HeiganDanceMeleeAction::Execute(Event event)
{
    CalculateSafe();
    if (!platform_phase && botAI->IsMainTank(bot) && !AI_VALUE2(bool, "has aggro", "boss target"))
    {
        return false;
    }
    assert(curr_safe >= 0 && curr_safe <= 3);
    return MoveInside(bot->GetMapId(), waypoints[curr_safe].first, waypoints[curr_safe].second, arenaZ,
                      botAI->IsMainTank(bot) ? 0 : 0, MovementPriority::MOVEMENT_COMBAT);
}

bool HeiganDanceRangedAction::Execute(Event event)
{
    CalculateSafe();
    if (!platform_phase)
    {
        Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
        bool tooCloseToBoss = boss && bot->IsWithinDistInMap(boss, 20.0f);
        bool onPlatform = bot->IsWithinDist2d(platform.first, platform.second, 3.0f);

        if (!onPlatform || tooCloseToBoss)
        {
            if (MoveTo(bot->GetMapId(), platform.first, platform.second, platformZ, false, false, false, false,
                       MovementPriority::MOVEMENT_COMBAT))
            {
                return true;
            }
            return MoveInside(bot->GetMapId(), platform.first, platform.second, platformZ, 2.0f,
                              MovementPriority::MOVEMENT_COMBAT);
        }
        return false;
    }
    botAI->InterruptSpell();
        return MoveInside(bot->GetMapId(), waypoints[curr_safe].first, waypoints[curr_safe].second, arenaZ, 0,
                      MovementPriority::MOVEMENT_COMBAT);
}

Unit* HeiganDispelDecrepitFeverAction::GetDecrepitFeverTarget() const
{
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    // Prioritize the main tank if possible.
    Unit* best = nullptr;
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (!member || !member->IsAlive())
            continue;

        if (!member->HasAura(NaxxSpellIds::DecrepitFever))
            continue;

        if (!bot->IsWithinDistInMap(member, botAI->GetRange("heal")))
            continue;

        if (botAI->IsMainTank(member))
            return member;

        // Keep first match as fallback.
        if (!best)
            best = member;
    }
    return best;
}

bool HeiganDispelDecrepitFeverAction::CanDispelDisease() const
{
    if (!bot->IsAlive())
    {
        return false;
    }

    // Keep it simple: only classes that can dispel disease in WotLK.
    switch (bot->getClass())
    {
        case CLASS_PALADIN:
            return botAI->CanCastSpell("cleanse", bot) || botAI->CanCastSpell("purify", bot);
        case CLASS_PRIEST:
            return botAI->CanCastSpell("cure disease", bot) || botAI->CanCastSpell("abolish disease", bot);
        case CLASS_SHAMAN:
            return botAI->CanCastSpell("cure disease", bot) || botAI->CanCastSpell("cleanse spirit", bot);
        default:
            return false;
    }
}

bool HeiganDispelDecrepitFeverAction::isUseful()
{
    Unit* heigan = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!heigan)
    {
        return false;
    }
    return CanDispelDisease() && GetDecrepitFeverTarget();
}

bool HeiganDispelDecrepitFeverAction::Execute(Event event)
{
    Unit* target = GetDecrepitFeverTarget();
    if (!target)
    {
        return false;
    }
    if (bot->getClass() == CLASS_PALADIN)
    {
        if (botAI->CanCastSpell("cleanse", target) && botAI->CastSpell("cleanse", target))
        {
            return true;
        }
        return botAI->CanCastSpell("purify", target) && botAI->CastSpell("purify", target);
    }
    if (botAI->CanCastSpell("cure disease", target) && botAI->CastSpell("cure disease", target))
    {
        return true;
    }

    if (botAI->CanCastSpell("cleanse spirit", target) && botAI->CastSpell("cleanse spirit", target))
    {
        return true;
    }

    return botAI->CanCastSpell("abolish disease", target) && botAI->CastSpell("abolish disease", target);
}