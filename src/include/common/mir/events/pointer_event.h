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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_COMMON_POINTER_EVENT_H_
#define MIR_COMMON_POINTER_EVENT_H_

#include "mir/events/input_event.h"

struct MirPointerEvent : MirInputEvent
{
    MirPointerEvent();
    MirPointerEvent(MirInputDeviceId dev,
                    std::chrono::nanoseconds et,
                    MirInputEventModifiers mods,
                    std::vector<uint8_t> const& cookie,
                    MirPointerAction action,
                    MirPointerButtons buttons,
                    float x,
                    float y,
                    float dx,
                    float dy,
                    float vscroll,
                    float hscroll);

    float x() const;
    void set_x(float x);

    float y() const;
    void set_y(float y);

    float dx() const;
    void set_dx(float x);

    float dy() const;
    void set_dy(float y);

    float vscroll() const;
    void set_vscroll(float v);

    float hscroll() const;
    void set_hscroll(float h);

    MirPointerAction action() const;
    void set_action(MirPointerAction action);

    MirPointerButtons buttons() const;
    void set_buttons(MirPointerButtons buttons);
private:
};

#endif
