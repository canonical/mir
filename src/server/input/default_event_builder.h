/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_DEFAULT_EVENT_BUILDER_H_
#define MIR_INPUT_DEFAULT_EVENT_BUILDER_H_

#include "mir/input/event_builder.h"
#include <memory>

namespace mir
{
namespace input
{
class DefaultEventBuilder : public EventBuilder
{
public:
    explicit DefaultEventBuilder(MirInputDeviceId device_id);

    EventUPtr key_event(Timestamp timestamp, MirKeyboardAction action, xkb_keysym_t key_code, int scan_code,
                        MirInputEventModifiers modifiers) override;

    EventUPtr touch_event(Timestamp timestamp, MirInputEventModifiers modifiers) override;
    void add_touch(MirEvent& event, MirTouchId touch_id, MirTouchAction action, MirTouchTooltype tooltype,
                   float x_axis_value, float y_axis_value, float pressure_value, float touch_major_value,
                   float touch_minor_value, float size_value) override;

    EventUPtr pointer_event(Timestamp timestamp, MirInputEventModifiers modifiers, MirPointerAction action,
                            MirPointerButtons buttons_pressed, float x_axis_value, float y_axis_value,
                            float hscroll_value, float vscroll_value, float relative_x_value,
                            float relative_y_value) override;

    EventUPtr configuration_event(Timestamp timestamp, MirInputConfigurationAction action) override;

private:
    MirInputDeviceId device_id;
};
}
}

#endif
