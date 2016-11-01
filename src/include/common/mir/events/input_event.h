/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_COMMON_INPUT_EVENT_H_
#define MIR_COMMON_INPUT_EVENT_H_

#include "mir/events/event.h"

struct MirInputEvent : MirEvent
{
    MirKeyboardEvent* to_keyboard();
    MirKeyboardEvent const* to_keyboard() const;

    MirMotionEvent* to_motion();
    MirMotionEvent const* to_motion() const;

protected:
    MirInputEvent() = default;
    MirInputEvent(MirInputEvent const& event) = default;
    MirInputEvent& operator=(MirInputEvent const& event) = default;
};

#endif /* MIR_COMMON_INPUT_EVENT_H_ */
