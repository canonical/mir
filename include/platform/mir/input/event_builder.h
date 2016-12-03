/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_EVENT_BUILDER_H_
#define MIR_INPUT_EVENT_BUILDER_H_

#include "mir_toolkit/event.h"
#include "mir/events/contact_state.h"
#include <memory>
#include <chrono>
#include <vector>

namespace mir
{

using EventUPtr = std::unique_ptr<MirEvent, void(*)(MirEvent*)>;

namespace input
{
class EventBuilder
{
public:
    EventBuilder() = default;
    virtual ~EventBuilder() = default;
    using Timestamp = std::chrono::nanoseconds;

    virtual EventUPtr key_event(Timestamp timestamp, MirKeyboardAction action, xkb_keysym_t key_code, int scan_code) = 0;

    virtual EventUPtr pointer_event(Timestamp timestamp, MirPointerAction action, MirPointerButtons buttons_pressed,
                                    float hscroll_value, float vscroll_value, float relative_x_value,
                                    float relative_y_value) = 0;

    virtual EventUPtr device_state_event(float cursor_x, float cursor_y) = 0;

    virtual EventUPtr pointer_event(Timestamp timestamp, MirPointerAction action, MirPointerButtons buttons_pressed,
                                    float x_position, float y_position,
                                    float hscroll_value, float vscroll_value, float relative_x_value,
                                    float relative_y_value) = 0;

    virtual EventUPtr touch_event(Timestamp timestamp, std::vector<mir::events::ContactState> const& contacts) = 0;
protected:
    EventBuilder(EventBuilder const&) = delete;
    EventBuilder& operator=(EventBuilder const&) = delete;
};
}
}

#endif
