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

#ifndef MIR_INPUT_DEFAULT_EVENT_BUILDER_H_
#define MIR_INPUT_DEFAULT_EVENT_BUILDER_H_

#include "mir/input/event_builder.h"
#include <memory>

namespace mir
{
namespace cookie
{
class Authority;
}
namespace input
{
class Seat;

class DefaultEventBuilder : public EventBuilder
{
public:
    explicit DefaultEventBuilder(MirInputDeviceId device_id,
                                 std::shared_ptr<cookie::Authority> const& cookie_authority,
                                 std::shared_ptr<Seat> const& seat);

    EventUPtr key_event(Timestamp timestamp, MirKeyboardAction action, xkb_keysym_t key_code, int scan_code) override;

    EventUPtr touch_event(Timestamp timestamp, std::vector<events::ContactState> const& contacts) override;

    EventUPtr pointer_event(Timestamp timestamp, MirPointerAction action, MirPointerButtons buttons_pressed,
                            float hscroll_value, float vscroll_value, float relative_x_value,
                            float relative_y_value) override;

    EventUPtr device_state_event(float cursor_x, float cursor_y) override;

    EventUPtr pointer_event(Timestamp timestamp, MirPointerAction action, MirPointerButtons buttons_pressed,
                            float x, float y, float hscroll_value, float vscroll_value, float relative_x_value,
                            float relative_y_value) override;


private:
    MirInputDeviceId const device_id;
    std::shared_ptr<cookie::Authority> const cookie_authority;
    std::shared_ptr<Seat> const seat;
};
}
}

#endif
