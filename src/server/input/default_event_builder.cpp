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
#include "mir/cookie_authority.h"
#include "mir/events/event_private.h"

#include <algorithm>

namespace me = mir::events;
namespace mi = mir::input;

mi::DefaultEventBuilder::DefaultEventBuilder(MirInputDeviceId device_id,
                                             std::shared_ptr<mir::cookie::CookieAuthority> const& cookie_authority)
    : device_id(device_id),
      cookie_authority(cookie_authority)
{
}

mir::EventUPtr mi::DefaultEventBuilder::key_event(Timestamp timestamp, MirKeyboardAction action, xkb_keysym_t key_code,
                                                  int scan_code)
{
    auto const& cookie = cookie_authority->timestamp_to_cookie(timestamp.count());
    return me::make_event(device_id, timestamp, cookie->marshall(), action, key_code, scan_code, mir_input_event_modifier_none);
}

mir::EventUPtr mi::DefaultEventBuilder::touch_event(Timestamp timestamp)
{
    return me::make_event(device_id, timestamp, std::vector<uint8_t>{}, mir_input_event_modifier_none);
}

void mi::DefaultEventBuilder::add_touch(MirEvent& event, MirTouchId touch_id, MirTouchAction action,
                                        MirTouchTooltype tooltype, float x_axis_value, float y_axis_value,
                                        float pressure_value, float touch_major_value, float touch_minor_value,
                                        float size_value)
{
    if (action == mir_touch_action_up || action == mir_touch_action_down)
    {
        auto const& cookie = cookie_authority->timestamp_to_cookie(event.motion.event_time.count());
        auto const& marshalled_cookie = cookie->marshall();
        std::copy_n(std::begin(marshalled_cookie), event.motion.cookie.size(), std::begin(event.motion.cookie));
    }

    me::add_touch(event, touch_id, action, tooltype, x_axis_value, y_axis_value, pressure_value, touch_major_value,
                  touch_minor_value, size_value);
}

mir::EventUPtr mi::DefaultEventBuilder::pointer_event(Timestamp timestamp, MirPointerAction action,
                                                      MirPointerButtons buttons_pressed, float hscroll_value, float vscroll_value,
                                                      float relative_x_value, float relative_y_value)
{
    const float x_axis_value = 0;
    const float y_axis_value = 0;
    std::vector<uint8_t> vec_cookie{};
    // FIXME Moving to 160bits soon
    if (action == mir_pointer_action_button_up || action == mir_pointer_action_button_down)
    {
        auto const& cookie = cookie_authority->timestamp_to_cookie(timestamp.count());
        vec_cookie = cookie->marshall();
    }
    return me::make_event(device_id, timestamp, vec_cookie, mir_input_event_modifier_none, action, buttons_pressed, x_axis_value, y_axis_value,
                          hscroll_value, vscroll_value, relative_x_value, relative_y_value);
}

mir::EventUPtr mi::DefaultEventBuilder::configuration_event(Timestamp timestamp, MirInputConfigurationAction action)
{
    return me::make_event(action, device_id, timestamp);
}
