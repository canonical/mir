/*
 * Copyright © Canonical Ltd.
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
 */

#include "input_method_grab_keyboard_v2.h"
#include "wl_seat.h"

#include <mir/input/composite_event_filter.h>
#include <mir/input/event_filter.h>
#include <mir/events/keyboard_event.h>
#include <mir/events/event_builders.h>
#include <mir/wayland/client.h>
#include <mir/executor.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;
namespace mev = mir::events;

class mf::InputMethodGrabKeyboardV2::Handler
    : public input::EventFilter
{
public:
    Handler(InputMethodGrabKeyboardV2* keyboard, std::shared_ptr<Executor> const& wayland_executor)
        : keyboard{keyboard},
          wayland_executor{wayland_executor}
    {
    }

    // \return true indicates the event was consumed by the filter
    bool handle(MirEvent const& event) override
    {
        if (mir_event_get_type(&event) == mir_event_type_input)
        {
            auto const input_ev = mir_event_get_input_event(&event);
            auto const ev_type = mir_input_event_get_type(input_ev);
            if (ev_type == mir_input_event_type_key || ev_type == mir_input_event_type_keyboard_resync)
            {
                std::shared_ptr<MirEvent> owned_event = mev::clone_event(event);
                wayland_executor->spawn([owned_event, keyboard=keyboard]()
                    {
                        if (keyboard)
                        {
                            keyboard.value().helper->handle_event(owned_event);
                        }
                    });
                return true;
            }
        }
        return false;
    }

private:
    wayland::Weak<InputMethodGrabKeyboardV2> const keyboard;
    std::shared_ptr<Executor> const wayland_executor;
};

mf::InputMethodGrabKeyboardV2::InputMethodGrabKeyboardV2(
    wl_resource* resource,
    WlSeat& seat,
    std::shared_ptr<Executor> const& wayland_executor,
    input::CompositeEventFilter& event_filter)
    : wayland::InputMethodKeyboardGrabV2{resource, Version<1>()},
      handler{std::make_shared<Handler>(this, wayland_executor)},
      helper{seat.make_keyboard_helper(this)}
{
    event_filter.prepend(handler);
    // On cleanup the handler will be dropped and automatically removed from the filter
}

void mf::InputMethodGrabKeyboardV2::send_repeat_info(int32_t rate, int32_t delay)
{
    send_repeat_info_event(rate, delay);
}

void mf::InputMethodGrabKeyboardV2::send_keymap_xkb_v1(mir::Fd const& fd, size_t length)
{
    send_keymap_event(mw::Keyboard::KeymapFormat::xkb_v1, fd, length);
}

void mf::InputMethodGrabKeyboardV2::send_key(std::shared_ptr<MirKeyboardEvent const> const& event)
{
    if (!wl_client)
    {
        return;
    }
    auto const serial = wl_client.value().next_serial(event);
    auto const timestamp = mir_input_event_get_wayland_timestamp(event.get());
    int const scancode = event->scan_code();
    auto const state = (event->action() == mir_keyboard_action_down) ?
        mw::Keyboard::KeyState::pressed :
        mw::Keyboard::KeyState::released;
    send_key_event(serial, timestamp, scancode, state);
}

void mf::InputMethodGrabKeyboardV2::send_modifiers(MirXkbModifiers const& modifiers)
{
    auto const serial = client->next_serial(nullptr);
    send_modifiers_event(
        serial,
        modifiers.depressed,
        modifiers.latched,
        modifiers.locked,
        modifiers.effective_layout);
}
