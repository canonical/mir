/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/events/motion_event.h"

MirKeyboardEvent* MirInputEvent::to_keyboard()
{
    return static_cast<MirKeyboardEvent*>(this);
}

MirKeyboardEvent const* MirInputEvent::to_keyboard() const
{
    return static_cast<MirKeyboardEvent const*>(this);
}

MirMotionEvent* MirInputEvent::to_motion()
{
    return static_cast<MirMotionEvent*>(this);
}

MirMotionEvent const* MirInputEvent::to_motion() const
{
    return static_cast<MirMotionEvent const*>(this);
}
