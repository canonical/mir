/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include "mir/input/seat.h"
#include "mir/events/event_builders.h"
#include "mir/cookie/authority.h"

#include <algorithm>

namespace me = mir::events;
namespace mi = mir::input;

mi::DefaultEventBuilder::DefaultEventBuilder(MirInputDeviceId device_id,
                                             std::shared_ptr<mir::cookie::Authority> const& cookie_authority,
                                             std::shared_ptr<mi::Seat> const& seat)
    : device_id(device_id),
      cookie_authority(cookie_authority),
      seat(seat)
{
}

mir::EventUPtr mi::DefaultEventBuilder::key_event(Timestamp timestamp, MirKeyboardAction action, xkb_keysym_t key_code,
                                                  int scan_code)
{
    auto const cookie = cookie_authority->make_cookie(timestamp.count());
    return me::make_event(device_id, timestamp, cookie->serialize(), action, key_code, scan_code, mir_input_event_modifier_none);
}

mir::EventUPtr mi::DefaultEventBuilder::pointer_event(Timestamp timestamp, MirPointerAction action,
                                                      MirPointerButtons buttons_pressed, float hscroll_value, float vscroll_value,
                                                      float relative_x_value, float relative_y_value)
{
    const float x_axis_value = 0;
    const float y_axis_value = 0;
    std::vector<uint8_t> vec_cookie{};
    if (action == mir_pointer_action_button_up || action == mir_pointer_action_button_down)
    {
        auto const cookie = cookie_authority->make_cookie(timestamp.count());
        vec_cookie = cookie->serialize();
    }
    return me::make_event(device_id, timestamp, vec_cookie, mir_input_event_modifier_none, action, buttons_pressed, x_axis_value, y_axis_value,
                          hscroll_value, vscroll_value, relative_x_value, relative_y_value);
}

mir::EventUPtr mi::DefaultEventBuilder::device_state_event(float cursor_x, float cursor_y)
{
    seat->set_cursor_position(cursor_x, cursor_y);
    return seat->create_device_state();
}

mir::EventUPtr mi::DefaultEventBuilder::pointer_event(Timestamp timestamp,
                                                      MirPointerAction action,
                                                      MirPointerButtons buttons_pressed,
                                                      float x_axis,
                                                      float y_axis,
                                                      float hscroll_value,
                                                      float vscroll_value,
                                                      float relative_x_value,
                                                      float relative_y_value)
{
    std::vector<uint8_t> vec_cookie{};
    if (action == mir_pointer_action_button_up || action == mir_pointer_action_button_down)
    {
        auto const cookie = cookie_authority->make_cookie(timestamp.count());
        vec_cookie = cookie->serialize();
    }
    return me::make_event(device_id, timestamp, vec_cookie, mir_input_event_modifier_none, action, buttons_pressed, x_axis, y_axis,
                          hscroll_value, vscroll_value, relative_x_value, relative_y_value);
}

mir::EventUPtr mi::DefaultEventBuilder::touch_event(Timestamp timestamp, std::vector<events::ContactState> const& contacts)
{
    std::vector<uint8_t> vec_cookie{};
    for (auto const& contact : contacts)
    {
        if (contact.action == mir_touch_action_up || contact.action == mir_touch_action_down)
        {
            auto const cookie = cookie_authority->make_cookie(timestamp.count());
            vec_cookie = cookie->serialize();
            break;
        }
    }
    return me::make_event(device_id, timestamp, vec_cookie, mir_input_event_modifier_none, contacts);
}
