/*
 * Copyright © 2018-2019 Canonical Ltd.
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

#include "wayland_wrapper.h"

#include <unordered_map>
#include <vector>
#include <functional>

namespace mir
{
namespace input
{
class InputDeviceHub;
class Seat;
class Keymap;
}
namespace time
{
class Clock;
}
namespace frontend
{
class WlPointer;
class WlKeyboard;
class WlTouch;
class WlSurface;

class WlSeat : public wayland::Seat::Global
{
public:
    WlSeat(
        wl_display* display,
        std::shared_ptr<time::Clock> const& clock,
        std::shared_ptr<mir::input::InputDeviceHub> const& input_hub,
        std::shared_ptr<mir::input::Seat> const& seat,
        bool enable_key_repeat);

    ~WlSeat();

    static auto from(struct wl_resource* resource) -> WlSeat*;

    void for_each_listener(wl_client* client, std::function<void(WlPointer*)> func);
    void for_each_listener(wl_client* client, std::function<void(WlKeyboard*)> func);
    void for_each_listener(wl_client* client, std::function<void(WlTouch*)> func);

    class FocusListener
    {
    public:
        /// The surface that has focus if the currently focused surface belongs to the relevant client. nullptr if there
        /// is no focused surface or it belongs to a different client.
        virtual void focus_on(WlSurface* surface) = 0;

        FocusListener() = default;
        virtual ~FocusListener() = default;
        FocusListener(FocusListener const&) = delete;
        FocusListener& operator=(FocusListener const&) = delete;
    };

    /// Adds the listener for future use, and makes a call into it to inform of initial state
    void add_focus_listener(wl_client* client, FocusListener* listener);
    void remove_focus_listener(wl_client* client, FocusListener* listener);
    void notify_focus(WlSurface& surface, bool has_focus);

    void server_restart();

private:
    void set_focus_to(WlSurface* surface);

    wl_client* focused_client{nullptr}; ///< Can be null
    wayland::Weak<WlSurface> focused_surface;
    wayland::DestroyListenerId focused_surface_destroy_listener_id{};

    template<class T>
    class ListenerList;

    class ConfigObserver;
    class Instance;

    std::shared_ptr<mir::input::Keymap> keymap;
    std::shared_ptr<ConfigObserver> const config_observer;

    // listener list are shared pointers so devices can keep them around long enough to remove themselves
    std::shared_ptr<ListenerList<FocusListener>> const focus_listeners;
    std::shared_ptr<ListenerList<WlPointer>> const pointer_listeners;
    std::shared_ptr<ListenerList<WlKeyboard>> const keyboard_listeners;
    std::shared_ptr<ListenerList<WlTouch>> const touch_listeners;

    std::shared_ptr<time::Clock> const clock;
    std::shared_ptr<input::InputDeviceHub> const input_hub;
    std::shared_ptr<input::Seat> const seat;
    bool const enable_key_repeat;

    void bind(wl_resource* new_wl_seat) override;
};
}
}

#endif // MIR_FRONTEND_WL_SEAT_H
