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

#include "default_event_builder.h"
#include "mir/events/event_builders.h"

namespace me = mir::events;
namespace mi = mir::input;

mi::DefaultEventBuilder::DefaultEventBuilder(MirInputDeviceId device_id) : device_id(device_id)
{
}

mir::EventUPtr mi::DefaultEventBuilder::key_event(Timestamp timestamp, MirKeyboardAction action, xkb_keysym_t key_code,
                                                  int scan_code, MirInputEventModifiers modifiers)
{
    uint64_t mac = 0;
    return me::make_event(device_id, timestamp, mac, action, key_code, scan_code, modifiers);
}

mir::EventUPtr mi::DefaultEventBuilder::touch_event(Timestamp timestamp, MirInputEventModifiers modifiers)
{
    uint64_t mac = 0;
    return me::make_event(device_id, timestamp, mac, modifiers);
}

void mi::DefaultEventBuilder::add_touch(MirEvent& event, MirTouchId touch_id, MirTouchAction action,
                                                  MirTouchTooltype tooltype, float x_axis_value, float y_axis_value,
                                                  float pressure_value, float touch_major_value,
                                                  float touch_minor_value, float size_value)
{
    me::add_touch(event, touch_id, action, tooltype, x_axis_value, y_axis_value, pressure_value, touch_major_value,
                  touch_minor_value, size_value);
}

mir::EventUPtr mi::DefaultEventBuilder::pointer_event(Timestamp timestamp, MirInputEventModifiers modifiers,
                                                      MirPointerAction action, MirPointerButtons buttons_pressed,
                                                      float x_axis_value, float y_axis_value, float hscroll_value,
                                                      float vscroll_value, float relative_x_value,
                                                      float relative_y_value)
{
    uint64_t mac = 0;
    return me::make_event(device_id, timestamp, mac, modifiers, action, buttons_pressed, x_axis_value, y_axis_value,
                          hscroll_value, vscroll_value, relative_x_value, relative_y_value);
}

mir::EventUPtr mi::DefaultEventBuilder::configuration_event(Timestamp timestamp, MirInputConfigurationAction action)
{
    return me::make_event(action, device_id, timestamp);
}
