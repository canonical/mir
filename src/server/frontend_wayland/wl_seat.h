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

#include "wayland.h"
#include "input_trigger_registry.h"
#include "wayland_wrapper.h"

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
namespace shell
{
class AccessibilityManager;
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
class SurfaceRegistry;
class InputTriggerRegistry;
class KeyboardStateTracker;

class PointerEventDispatcher
{
public:
    explicit PointerEventDispatcher(std::shared_ptr<WlPointer> const& wl_pointer);

    void event(std::shared_ptr<MirPointerEvent const> const& event, WlSurface& root_surface);

    void start_dispatch_to_data_device(WlDataDevice* wl_data_device);
    void stop_dispatch_to_data_device();
private:
    std::weak_ptr<WlPointer> wl_pointer;
    wayland::Weak<WlDataDevice> wl_data_device;
};

class WlSeat : public wayland::Seat::Global
{
public:
    WlSeat(
        std::shared_ptr<time::Clock> const& clock,
        std::shared_ptr<mir::input::InputDeviceHub> const& input_hub,
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
        std::shared_ptr<mir::input::Seat> const& seat,
        std::shared_ptr<shell::AccessibilityManager> const& accessibility_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry,
        std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry,
        std::shared_ptr<KeyboardStateTracker> const& keyboard_state_tracker);

    ~WlSeat();

    WlSeat(WlSeat const&) = delete;
    WlSeat& operator=(WlSeat const&) = delete;
    WlSeat(WlSeat&&) = delete;
    WlSeat& operator=(WlSeat&&) = delete;

    static auto from(struct wl_resource* resource) -> WlSeat*;

    void for_each_listener(rust::Box<wayland_rs::WaylandClient> const* client, std::function<void(PointerEventDispatcher*)> func);
    void for_each_listener(rust::Box<wayland_rs::WaylandClient> const* client, std::function<void(WlKeyboard*)> func);
    void for_each_listener(rust::Box<wayland_rs::WaylandClient> const* client, std::function<void(WlTouch*)> func);

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
        FocusListener(FocusListener&&) = delete;
        FocusListener& operator=(FocusListener&&) = delete;
    };

    auto make_keyboard_helper(KeyboardCallbacks* callbacks) -> std::shared_ptr<KeyboardHelper>;

    /// Adds the listener for future use, and makes a call into it to inform of initial state
    void add_focus_listener(rust::Box<wayland_rs::WaylandClient> const* client, FocusListener* listener);
    void remove_focus_listener(rust::Box<wayland_rs::WaylandClient> const* client, FocusListener* listener);

    auto create(rust::Box<wayland_rs::WaylandClient> client) -> std::shared_ptr<wayland_rs::WlSeatImpl>;

private:
    void set_focus_to(WlSurface* surface);

    rust::Box<wayland_rs::WaylandClient>* focused_client{nullptr}; ///< Can be null
    wayland_rs::Weak<WlSurface> focused_surface;
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

    std::shared_ptr<shell::AccessibilityManager> const accessibility_manager;
};
}
}

#endif // MIR_FRONTEND_WL_SEAT_H
