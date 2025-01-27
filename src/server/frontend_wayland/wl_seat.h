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

#ifndef MIR_FRONTEND_WL_SEAT_H
#define MIR_FRONTEND_WL_SEAT_H

#include "wayland_wrapper.h"
#include "mir/wayland/weak.h"

#include <functional>

struct MirPointerEvent;

namespace mir
{
class Executor;
template<typename T>
class ObserverRegistrar;
namespace input
{
class InputDeviceHub;
class Seat;
class Keymap;
class KeyboardObserver;
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
class WlDataDevice;
class KeyboardCallbacks;
class KeyboardHelper;

class PointerEventDispatcher
{
public:
    explicit PointerEventDispatcher(WlPointer* wl_pointer);

    void event(std::shared_ptr<MirPointerEvent const> const& event, WlSurface& root_surface);

    void start_dispatch_to_data_device(WlDataDevice* wl_data_device);
    void stop_dispatch_to_data_device();
private:
    wayland::Weak<WlPointer> wl_pointer;
    wayland::Weak<WlDataDevice> wl_data_device;
};

class WlSeat : public wayland::Seat::Global
{
public:
    WlSeat(
        wl_display* display,
        Executor& wayland_executor,
        std::shared_ptr<time::Clock> const& clock,
        std::shared_ptr<mir::input::InputDeviceHub> const& input_hub,
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
        std::shared_ptr<mir::input::Seat> const& seat,
        bool enable_key_repeat);

    ~WlSeat();

    static auto from(struct wl_resource* resource) -> WlSeat*;

    void for_each_listener(wayland::Client* client, std::function<void(PointerEventDispatcher*)> func);
    void for_each_listener(wayland::Client* client, std::function<void(WlKeyboard*)> func);
    void for_each_listener(wayland::Client* client, std::function<void(WlTouch*)> func);

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

    auto make_keyboard_helper(KeyboardCallbacks* callbacks) -> std::shared_ptr<KeyboardHelper>;

    /// Adds the listener for future use, and makes a call into it to inform of initial state
    void add_focus_listener(wayland::Client* client, FocusListener* listener);
    void remove_focus_listener(wayland::Client* client, FocusListener* listener);

private:
    void set_focus_to(WlSurface* surface);

    wayland::Client* focused_client{nullptr}; ///< Can be null
    wayland::Weak<WlSurface> focused_surface;
    wayland::DestroyListenerId focused_surface_destroy_listener_id{};

    template<class T>
    class ListenerList;

    class ConfigObserver;
    class Instance;
    class KeyboardObserver;

    std::shared_ptr<mir::input::Keymap> keymap;
    std::shared_ptr<ConfigObserver> const config_observer;
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const keyboard_observer_registrar;
    std::shared_ptr<KeyboardObserver> const keyboard_observer;

    // listener list are shared pointers so devices can keep them around long enough to remove themselves
    std::shared_ptr<ListenerList<FocusListener>> const focus_listeners;
    std::shared_ptr<ListenerList<PointerEventDispatcher>> const pointer_listeners;
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
