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

#include "wayland_wrapper.h"

#include "wl_seat.h"

#include "wayland_utils.h"
#include "wl_surface.h"
#include "wl_keyboard.h"
#include "wl_pointer.h"
#include "wl_touch.h"
#include "wl_data_device.h"

#include "mir/executor.h"
#include "mir/wayland/client.h"
#include "mir/observer_registrar.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/device.h"
#include "mir/input/parameter_keymap.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/input/keyboard_observer.h"
#include "mir/scene/surface.h"
#include "mir/shell/accessibility_manager.h"
#include "mir_toolkit/events/input/pointer_event.h"

#include <mutex>
#include <algorithm>

namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace mw = mir::wayland;
namespace mev = mir::events;

template<class T>
class mf::WlSeat::ListenerList
{
public:
    ListenerList() = default;

    ListenerList(ListenerList&&) = delete;
    ListenerList(ListenerList const&) = delete;
    ListenerList& operator=(ListenerList const&) = delete;

    void register_listener(mw::Client* client, T* listener)
    {
        listeners[client].push_back(listener);
    }

    void unregister_listener(mw::Client* client, T const* listener)
    {
        std::vector<T*>& client_listeners = listeners[client];
        client_listeners.erase(
            std::remove(
                client_listeners.begin(),
                client_listeners.end(),
                listener),
            client_listeners.end());
        if (client_listeners.size() == 0)
            listeners.erase(client);
    }

    void for_each(mw::Client* client, std::function<void(T*)> func)
    {
        for (auto listener: listeners[client])
            func(listener);
    }

private:
    std::unordered_map<mw::Client*, std::vector<T*>> listeners;
};

class mf::WlSeat::ConfigObserver : public mi::InputDeviceObserver
{
public:
    ConfigObserver(
        std::shared_ptr<mi::Keymap> const& keymap,
        std::function<void(std::shared_ptr<mi::Keymap> const&)> const& on_keymap_commit)
        : current_keymap{keymap},
          pending_keymap{nullptr},
          on_keymap_commit{on_keymap_commit}
    {
    }

    void device_added(std::shared_ptr<input::Device> const& device) override;
    void device_changed(std::shared_ptr<input::Device> const& device) override;
    void device_removed(std::shared_ptr<input::Device> const& device) override;
    void changes_complete() override;

private:
    std::shared_ptr<mi::Keymap> current_keymap;
    std::shared_ptr<mi::Keymap> pending_keymap;
    std::function<void(std::shared_ptr<mi::Keymap> const&)> const on_keymap_commit;
};

void mf::WlSeat::ConfigObserver::device_added(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        pending_keymap = keyboard_config.value().device_keymap();
    }
}

void mf::WlSeat::ConfigObserver::device_changed(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        pending_keymap = keyboard_config.value().device_keymap();
    }
}

void mf::WlSeat::ConfigObserver::device_removed(std::shared_ptr<input::Device> const& /*device*/)
{
}

void mf::WlSeat::ConfigObserver::changes_complete()
{
    if (pending_keymap && !pending_keymap->matches(*current_keymap))
    {
        current_keymap = std::move(pending_keymap);
        on_keymap_commit(current_keymap);
    }

    pending_keymap.reset();
}

class mf::WlSeat::KeyboardObserver
    : public input::KeyboardObserver
{
public:
    KeyboardObserver(WlSeat& seat)
        : seat{seat}
    {
    }

    void keyboard_event(std::shared_ptr<MirEvent const> const& event) override
    {
        if (seat.focused_surface)
        {
            seat.for_each_listener(seat.focused_surface.value().client, [&](WlKeyboard* keyboard)
                {
                    keyboard->handle_event(event);
                });
        }
    }

    void keyboard_focus_set(std::shared_ptr<mi::Surface> const& surface) override
    {
        if (auto const scene_surface = dynamic_cast<ms::Surface*>(surface.get()))
        {
            seat.set_focus_to(mw::as_nullable_ptr(scene_surface->wayland_surface()));
        }
        else
        {
            seat.set_focus_to({});
        }
    }

private:
    WlSeat& seat;
};

class mf::WlSeat::Instance : public wayland::Seat
{
public:
    Instance(wl_resource* new_resource, mf::WlSeat* seat);

    mf::WlSeat* const seat;

private:
    void get_pointer(wl_resource* new_pointer) override;
    void get_keyboard(wl_resource* new_keyboard) override;
    void get_touch(wl_resource* new_touch) override;
};

mf::WlSeat::WlSeat(
    wl_display* display,
    Executor& wayland_executor,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<mi::InputDeviceHub> const& input_hub,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<shell::AccessibilityManager> const& accessibility_manager)
    :   Global(display, Version<9>()),
        keymap{std::make_shared<input::ParameterKeymap>()},
        config_observer{
            std::make_shared<ConfigObserver>(
                keymap,
                [this](std::shared_ptr<mi::Keymap> const& new_keymap)
                {
                    keymap = new_keymap;
                })},
        keyboard_observer_registrar{keyboard_observer_registrar},
        keyboard_observer{std::make_shared<KeyboardObserver>(*this)},
        focus_listeners{std::make_shared<ListenerList<FocusListener>>()},
        pointer_listeners{std::make_shared<ListenerList<PointerEventDispatcher>>()},
        keyboard_listeners{std::make_shared<ListenerList<WlKeyboard>>()},
        touch_listeners{std::make_shared<ListenerList<WlTouch>>()},
        clock{clock},
        input_hub{input_hub},
        seat{seat},
        accessibility_manager{accessibility_manager}
{
    input_hub->add_observer(config_observer);
    keyboard_observer_registrar->register_interest(keyboard_observer, wayland_executor);
}

mf::WlSeat::~WlSeat()
{
    keyboard_observer_registrar->unregister_interest(*keyboard_observer);
    input_hub->remove_observer(config_observer);
    if (focused_surface)
    {
        focused_surface.value().remove_destroy_listener(focused_surface_destroy_listener_id);
    }
}

auto mf::WlSeat::from(struct wl_resource* resource) -> WlSeat*
{
    auto const instance = static_cast<mf::WlSeat::Instance*>(wayland::Seat::from(resource));
    return instance ? instance->seat : nullptr;
}

void mf::WlSeat::for_each_listener(mw::Client* client, std::function<void(PointerEventDispatcher*)> func)
{
    pointer_listeners->for_each(client, func);
}

void mf::WlSeat::for_each_listener(mw::Client* client, std::function<void(WlKeyboard*)> func)
{
    keyboard_listeners->for_each(client, func);
}

void mf::WlSeat::for_each_listener(mw::Client* client, std::function<void(WlTouch*)> func)
{
    touch_listeners->for_each(client, func);
}

auto mf::WlSeat::make_keyboard_helper(KeyboardCallbacks* callbacks) -> std::shared_ptr<KeyboardHelper>
{
    // https://wayland.app/protocols/wayland#wl_keyboard:event:repeat_info
    // " A rate of zero will disable any repeating (regardless of the value of
    // delay)."
    auto const default_repeat_rate = accessibility_manager->repeat_rate();
    auto const default_repeat_delay = accessibility_manager->repeat_delay();
    auto const keyboard_helper = std::shared_ptr<KeyboardHelper>(new KeyboardHelper(callbacks, keymap, seat, default_repeat_rate, default_repeat_delay));
    accessibility_manager->register_keyboard_helper(keyboard_helper);
    return keyboard_helper;
}

void mf::WlSeat::bind(wl_resource* new_wl_seat)
{
    new Instance{new_wl_seat, this};
}

void mf::WlSeat::set_focus_to(WlSurface* new_surface)
{
    auto const new_client = new_surface ? new_surface->client : nullptr;
    if (new_client != focused_client)
    {
        keyboard_listeners->for_each(focused_client, [](WlKeyboard* keyboard)
            {
                keyboard->focus_on(nullptr);
            });
        focus_listeners->for_each(focused_client, [](FocusListener* listener)
            {
                listener->focus_on(nullptr);
            });
    }
    if (focused_surface)
    {
        focused_surface.value().remove_destroy_listener(focused_surface_destroy_listener_id);
    }
    focused_client = new_client;
    focused_surface = mw::make_weak(new_surface);
    if (new_surface)
    {
        // This listener will be removed when either the focus changes or the seat is destroyed
        focused_surface_destroy_listener_id = new_surface->add_destroy_listener([this]()
            {
                set_focus_to(nullptr);
            });
    }
    else
    {
        focused_surface_destroy_listener_id = {};
    }
    keyboard_listeners->for_each(focused_client, [&](WlKeyboard* keyboard)
        {
            keyboard->focus_on(new_surface);
        });
    focus_listeners->for_each(new_client, [&](FocusListener* listener)
        {
            listener->focus_on(new_surface);
        });
}

mf::WlSeat::Instance::Instance(wl_resource* new_resource, mf::WlSeat* seat)
    : mw::Seat(new_resource, Version<9>()),
      seat{seat}
{
    // TODO: Read the actual capabilities. Do we have a keyboard? Mouse? Touch?
    send_capabilities_event(Capability::pointer | Capability::keyboard | Capability::touch);
    send_name_event_if_supported("seat0");
}

void mf::WlSeat::Instance::get_pointer(wl_resource* new_pointer)
{
    auto const pointer = new WlPointer{new_pointer};
    auto dispatcher = std::make_shared<PointerEventDispatcher>(pointer);

    seat->pointer_listeners->register_listener(client, dispatcher.get());
    pointer->add_destroy_listener(
        [listeners = seat->pointer_listeners, listener = std::move(dispatcher), client = client]()
        {
            listeners->unregister_listener(client, listener.get());
        });
}

void mf::WlSeat::Instance::get_keyboard(wl_resource* new_keyboard)
{
    auto const keyboard = new WlKeyboard{new_keyboard, *seat};

    seat->keyboard_listeners->register_listener(client, keyboard);
    keyboard->add_destroy_listener(
        [listeners = seat->keyboard_listeners, listener = keyboard, client = client]()
        {
            listeners->unregister_listener(client, listener);
        });
}

void mf::WlSeat::Instance::get_touch(wl_resource* new_touch)
{
    auto const touch = new WlTouch{new_touch, seat->clock};
    seat->touch_listeners->register_listener(client, touch);
    touch->add_destroy_listener(
        [listeners = seat->touch_listeners, listener = touch, client = client]()
        {
            listeners->unregister_listener(client, listener);
        });
}

void mf::WlSeat::add_focus_listener(mw::Client* client, FocusListener* listener)
{
    focus_listeners->register_listener(client, listener);
    if (focused_client == client)
    {
        listener->focus_on(mw::as_nullable_ptr(focused_surface));
    }
    else
    {
        listener->focus_on(nullptr);
    }
}

void mf::WlSeat::remove_focus_listener(mw::Client* client, FocusListener* listener)
{
    focus_listeners->unregister_listener(client, listener);
}

mf::PointerEventDispatcher::PointerEventDispatcher(WlPointer* wl_pointer) :
    wl_pointer{wl_pointer}
{
}

void mf::PointerEventDispatcher::event(std::shared_ptr<MirPointerEvent const> const& event, WlSurface& root_surface)
{
    if (wl_data_device)
    {
        wl_data_device.value().event(event, root_surface);
    }
    else if (wl_pointer)
    {
        wl_pointer.value().event(event, root_surface);
    }
}

void mf::PointerEventDispatcher::start_dispatch_to_data_device(WlDataDevice* wl_data_device)
{
    this->wl_data_device = wayland::Weak<WlDataDevice>{wl_data_device};

    if (wl_pointer)
    {
        wl_pointer.value().leave(std::nullopt);
    }
}

void mf::PointerEventDispatcher::stop_dispatch_to_data_device()
{
    wl_data_device = wayland::Weak<WlDataDevice>{};
}
