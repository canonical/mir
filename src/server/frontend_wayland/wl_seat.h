/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_WL_SEAT_H
#define MIR_FRONTEND_WL_SEAT_H

#include "generated/wayland_wrapper.h"

#include <unordered_map>

// from "mir_toolkit/events/event.h"
struct MirInputEvent;
struct MirSurfaceEvent;
typedef struct MirSurfaceEvent MirWindowEvent;
struct MirKeymapEvent;

namespace mir
{
class Executor;

namespace input
{
class InputDeviceHub;
class Seat;
class Keymap;
}
namespace frontend
{
class WlPointer;
class WlKeyboard;
class WlTouch;

class WlSeat : public wayland::Seat
{
public:
    WlSeat(
        wl_display* display,
        std::shared_ptr<mir::input::InputDeviceHub> const& input_hub,
        std::shared_ptr<mir::input::Seat> const& seat,
        std::shared_ptr<mir::Executor> const& executor);

    ~WlSeat();

    void handle_pointer_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const;
    void handle_keyboard_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const;
    void handle_touch_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const;
    void handle_event(wl_client* client, MirKeymapEvent const* keymap_event, wl_resource* target) const;
    void handle_event(wl_client* client, MirWindowEvent const* window_event, wl_resource* target) const;

    void spawn(std::function<void()>&& work);

private:
    template<class T>
    class ListenerList;

    class ConfigObserver;

    std::unique_ptr<mir::input::Keymap> const keymap;
    std::shared_ptr<ConfigObserver> const config_observer;

    std::unique_ptr<std::unordered_map<wl_client*, ListenerList<WlPointer>>> const pointer_listeners;
    std::unique_ptr<std::unordered_map<wl_client*, ListenerList<WlKeyboard>>> const keyboard_listeners;
    std::unique_ptr<std::unordered_map<wl_client*, ListenerList<WlTouch>>> const touch_listeners;

    std::shared_ptr<input::InputDeviceHub> const input_hub;
    std::shared_ptr<input::Seat> const seat;

    std::shared_ptr<mir::Executor> const executor;

    void bind(wl_client* client, wl_resource* resource) override;
    void get_pointer(wl_client* client, wl_resource* resource, uint32_t id) override;
    void get_keyboard(wl_client* client, wl_resource* resource, uint32_t id) override;
    void get_touch(wl_client* client, wl_resource* resource, uint32_t id) override;
    void release(struct wl_client* /*client*/, struct wl_resource* us) override;
};
}
}

#endif // MIR_FRONTEND_WL_SEAT_H
