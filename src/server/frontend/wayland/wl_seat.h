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

#ifndef WL_SEAT_H
#define WL_SEAT_H

#include "basic_surface_event_sink.h"

#include <unordered_map>

// from <wayland-server-core.h>
struct wl_client;
struct wl_resource;

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
template<class InputInterface>
class InputCtx; // defined in wl_seat.cpp

class WlPointer;
class WlKeyboard;
class WlTouch;

class WlSeat : public BasicWlSeat
{
public:
    WlSeat(
        wl_display* display,
        std::shared_ptr<mir::input::InputDeviceHub> const& input_hub,
        std::shared_ptr<mir::input::Seat> const& seat,
        std::shared_ptr<mir::Executor> const& executor);

    ~WlSeat();

    void handle_pointer_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const override;
    void handle_keyboard_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const override;
    void handle_touch_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const override;
    void handle_event(wl_client* client, MirKeymapEvent const* keymap_event, wl_resource* target) const override;
    void handle_event(wl_client* client, MirWindowEvent const* window_event, wl_resource* target) const override;

    void spawn(std::function<void()>&& work) override;

private:
    class ConfigObserver;

    std::unique_ptr<mir::input::Keymap> const keymap;
    std::shared_ptr<ConfigObserver> const config_observer;

    std::unique_ptr<std::unordered_map<wl_client*, InputCtx<WlPointer>>> const pointer;
    std::unique_ptr<std::unordered_map<wl_client*, InputCtx<WlKeyboard>>> const keyboard;
    std::unique_ptr<std::unordered_map<wl_client*, InputCtx<WlTouch>>> const touch;

    std::shared_ptr<input::InputDeviceHub> const input_hub;
    std::shared_ptr<input::Seat> const seat;

    std::shared_ptr<mir::Executor> const executor;

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id);
    static void get_pointer(wl_client* client, wl_resource* resource, uint32_t id);
    static void get_keyboard(wl_client* client, wl_resource* resource, uint32_t id);
    static void get_touch(wl_client* client, wl_resource* resource, uint32_t id);
    static void release(struct wl_client* /*client*/, struct wl_resource* us);

    wl_global* const global;
    static struct wl_seat_interface const vtable;
};
}
}

#endif // WL_SEAT_H
