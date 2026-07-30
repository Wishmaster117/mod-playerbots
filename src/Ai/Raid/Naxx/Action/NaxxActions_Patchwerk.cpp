/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"

#include <algorithm>
#include <cmath>

bool PatchwerkRangedPositionAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "patchwerk");
    if (!boss)
        return false;

    constexpr float MinDistance = 12.0f;
    constexpr float MaxDistance = 15.0f;

    float const currentDistance = bot->GetExactDist2d(boss);
    if (currentDistance >= MinDistance && currentDistance <= MaxDistance)
        return false;

    float angle = boss->GetAngle(bot);
    if (currentDistance < 0.1f)
        angle = boss->GetOrientation();

    float const desiredDistance = std::clamp(currentDistance, MinDistance, MaxDistance);
    float const x = boss->GetPositionX() + std::cos(angle) * desiredDistance;
    float const y = boss->GetPositionY() + std::sin(angle) * desiredDistance;

    return MoveTo(boss->GetMapId(), x, y, bot->GetPositionZ(), false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT, true, false);
}
