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
#include <memory>
#include <chrono>

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

    virtual EventUPtr key_event(Timestamp timestamp, MirKeyboardAction action, xkb_keysym_t key_code, int scan_code,
                                MirInputEventModifiers modifiers) = 0;

    virtual EventUPtr touch_event(Timestamp timestamp, MirInputEventModifiers modifiers) = 0;
    virtual void add_touch(MirEvent& event, MirTouchId touch_id, MirTouchAction action, MirTouchTooltype tooltype,
                           float x_axis_value, float y_axis_value, float pressure_value, float touch_major_value,
                           float touch_minor_value, float size_value) = 0;

    virtual EventUPtr pointer_event(Timestamp timestamp, MirInputEventModifiers modifiers, MirPointerAction action,
                                    MirPointerButtons buttons_pressed, float x_axis_value, float y_axis_value,
                                    float hscroll_value, float vscroll_value, float relative_x_value,
                                    float relative_y_value) = 0;

    virtual EventUPtr configuration_event(Timestamp timestamp, MirInputConfigurationAction action) = 0;

protected:
    EventBuilder(EventBuilder const&) = delete;
    EventBuilder& operator=(EventBuilder const&) = delete;
};
}
}

#endif
