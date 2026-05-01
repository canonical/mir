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
#include "surface_registry.h"
#include "wayland.h"
#include "client.h"

#include "wl_seat.h"

#include "wayland_utils.h"
#include "wl_surface.h"
#include "wl_keyboard.h"
#include "wl_pointer.h"
#include "wl_touch.h"
#include "wl_data_device.h"
#include "keyboard_state_tracker.h"

#include <mir/executor.h>
#include <mir/observer_registrar.h>
#include <mir/input/input_device_observer.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/device.h>
#include <mir/input/parameter_keymap.h>
#include <mir/input/mir_keyboard_config.h>
#include <mir/input/keyboard_observer.h>
#include <mir/scene/surface.h>
#include <mir/shell/accessibility_manager.h>
#include <mir_toolkit/events/input/pointer_event.h>
#include <mir/events/keyboard_event.h>

#include <mutex>
#include <algorithm>

namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace mw = mir::wayland_rs;
namespace mev = mir::events;

template<class T>
class mf::WlSeatGlobal::ListenerList
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
    std::unordered_map<void*, std::vector<T*>> listeners;
};

class mf::WlSeatGlobal::ConfigObserver : public mi::InputDeviceObserver
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

void mf::WlSeatGlobal::ConfigObserver::device_added(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        pending_keymap = keyboard_config.value().device_keymap();
    }
}

void mf::WlSeatGlobal::ConfigObserver::device_changed(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        pending_keymap = keyboard_config.value().device_keymap();
    }
}

void mf::WlSeatGlobal::ConfigObserver::device_removed(std::shared_ptr<input::Device> const& /*device*/)
{
}

void mf::WlSeatGlobal::ConfigObserver::changes_complete()
{
    if (pending_keymap && !pending_keymap->matches(*current_keymap))
    {
        current_keymap = std::move(pending_keymap);
        on_keymap_commit(current_keymap);
    }

    pending_keymap.reset();
}

class mf::WlSeatGlobal::KeyboardObserver
    : public input::KeyboardObserver
{
public:
    KeyboardObserver(
        WlSeatGlobal& seat,
        std::shared_ptr<mf::SurfaceRegistry> const surface_registry,
        std::shared_ptr<InputTriggerRegistry> const& input_trigger_registry,
        std::shared_ptr<KeyboardStateTracker> const& keyboard_state_tracker) :
        seat{seat},
        surface_registry{surface_registry},
        input_trigger_registry{input_trigger_registry},
        keyboard_state_tracker{keyboard_state_tracker}
    {
    }

    class ConsumedKeyTracker
    {
    public:
        bool should_consume_event(bool handled, MirEvent const& event)
        {
            if (event.type() != mir_event_type_input || event.to_input()->input_type() != mir_input_event_type_key)
                return false;

            auto const key_event = *event.to_input()->to_keyboard();
            auto const scancode = key_event.scan_code();

            switch (key_event.action())
            {
                case mir_keyboard_action_down:
                    if (!handled)
                        return false;
                    consumed_down_scancodes.insert(scancode);
                    return true;

                case mir_keyboard_action_repeat:
                    return consumed_down_scancodes.contains(scancode);

                case mir_keyboard_action_up:
                    // If the down event was consumed, consume the up event.
                    // Otherwise, pass it to the client.
                    if (consumed_down_scancodes.contains(scancode))
                    {
                        consumed_down_scancodes.erase(scancode);
                        return true;
                    }

                    return false;
                default:
                    return false;
            }

            return false;
        }

    private:
        std::unordered_set<uint32_t> consumed_down_scancodes;
    };

    void keyboard_event(std::shared_ptr<MirEvent const> const& event) override
    {
        keyboard_state_tracker->process(*event);

        auto const handled = input_trigger_registry->any_trigger_handled(*event);

        // Track which events were handled and consumed, and which were passed
        // to the client. For consumed down events, their up counterparts will
        // be consumed. For non-consumed events, their up counterparts will be
        // passed to the client.
        if(consumed_key_tracker.should_consume_event(handled, *event))
            return;

        if (seat.focused_surface)
        {
            seat.for_each_listener(seat.focused_surface.value().client.lock().get(), [&](WlKeyboard* keyboard)
                {
                    keyboard->handle_event(event);
                });
        }
    }

    void keyboard_focus_set(std::shared_ptr<mi::Surface> const& surface) override
    {
        if (auto const wayland_surface = surface_registry->lookup_wayland_surface(surface))
        {
            seat.set_focus_to(&wayland_surface.value().value());
        }
        else
        {
            seat.set_focus_to({});
        }
    }

private:
    WlSeatGlobal& seat;
    std::shared_ptr<mf::SurfaceRegistry> const surface_registry;
    std::shared_ptr<mf::InputTriggerRegistry> const input_trigger_registry;
    std::shared_ptr<KeyboardStateTracker> const keyboard_state_tracker;
    ConsumedKeyTracker consumed_key_tracker;
};

class mf::WlSeatGlobal::Instance : public mw::WlSeatImpl
{
public:
    Instance(mf::WlSeatGlobal* seat, std::shared_ptr<mw::Client> client);
    auto associate(rust::Box<wayland_rs::WlSeatExt> instance, uint32_t object_id) -> void override;

    mf::WlSeatGlobal* const seat;
    std::shared_ptr<mw::Client> const client;

    auto get_pointer() -> std::shared_ptr<wayland_rs::WlPointerImpl> override;
    auto get_keyboard() -> std::shared_ptr<wayland_rs::WlKeyboardImpl> override;
    auto get_touch() -> std::shared_ptr<wayland_rs::WlTouchImpl> override;
};

mf::WlSeatGlobal::WlSeatGlobal(
    Executor& wayland_executor,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<mi::InputDeviceHub> const& input_hub,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<shell::AccessibilityManager> const& accessibility_manager,
    std::shared_ptr<mf::SurfaceRegistry> const& surface_registry,
    std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry,
    std::shared_ptr<KeyboardStateTracker> const& keyboard_state_tracker)
    :   keymap{std::make_shared<input::ParameterKeymap>()},
        config_observer{
            std::make_shared<ConfigObserver>(
                keymap,
                [this](std::shared_ptr<mi::Keymap> const& new_keymap)
                {
                    keymap = new_keymap;
                })},
        keyboard_observer_registrar{keyboard_observer_registrar},
        keyboard_observer{std::make_shared<KeyboardObserver>(*this, surface_registry, input_trigger_registry, keyboard_state_tracker)},
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

mf::WlSeatGlobal::~WlSeatGlobal()
{
    keyboard_observer_registrar->unregister_interest(*keyboard_observer);
    input_hub->remove_observer(config_observer);
    if (focused_surface)
    {
        focused_surface.value().remove_destroy_listener(focused_surface_destroy_listener_id);
    }
}

auto mf::WlSeatGlobal::from(wayland_rs::WlSeatImpl* impl) -> WlSeatGlobal*
{
    auto const instance = dynamic_cast<mf::WlSeatGlobal::Instance*>(impl);
    return instance ? instance->seat : nullptr;
}

void mf::WlSeatGlobal::for_each_listener(mw::Client* client, std::function<void(PointerEventDispatcher*)> func)
{
    pointer_listeners->for_each(client, func);
}

void mf::WlSeatGlobal::for_each_listener(mw::Client* client, std::function<void(WlKeyboard*)> func)
{
    keyboard_listeners->for_each(client, func);
}

void mf::WlSeatGlobal::for_each_listener(mw::Client* client, std::function<void(WlTouch*)> func)
{
    touch_listeners->for_each(client, func);
}

auto mf::WlSeatGlobal::make_keyboard_helper(KeyboardCallbacks* callbacks) -> std::shared_ptr<KeyboardHelper>
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

auto mir::frontend::WlSeatGlobal::create(std::shared_ptr<wayland_rs::Client> const& client) -> std::shared_ptr<wayland_rs::WlSeatImpl>
{
    return std::make_shared<Instance>(this, client);
}

void mf::WlSeatGlobal::set_focus_to(WlSurface* new_surface)
{
    auto const new_client = new_surface ? new_surface->client.lock().get() : nullptr;
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
    if (new_surface)
    {
        focused_surface = mw::Weak(new_surface->shared_from_this());
        // This listener will be removed when either the focus changes or the seat is destroyed
        focused_surface_destroy_listener_id = new_surface->add_destroy_listener([this]()
            {
                set_focus_to(nullptr);
            });
    }
    else
    {
        focused_surface = {};
        focused_surface_destroy_listener_id = {};
    }
    keyboard_listeners->for_each(focused_client, [&](WlKeyboard* keyboard)
        {
            keyboard->focus_on(new_surface);
        });
    focus_listeners->for_each(new_client, [&](FocusListener* listener)
        {
            listener->focus_on(new_surface ? new_surface->shared_from_this() : nullptr);
        });
}

mf::WlSeatGlobal::Instance::Instance(mf::WlSeatGlobal* seat, std::shared_ptr<mw::Client> client)
    : seat{seat},
      client{std::move(client)}
{
}

auto mir::frontend::WlSeatGlobal::Instance::associate(rust::Box<wayland_rs::WlSeatExt> instance, uint32_t object_id) -> void
{
    mw::WlSeatImpl::associate(std::move(instance), object_id);
    // TODO: Read the actual capabilities. Do we have a keyboard? Mouse? Touch?
    send_capabilities_event(Capability::pointer | Capability::keyboard | Capability::touch);
    send_name_event("seat0");
}

auto mf::WlSeatGlobal::Instance::get_pointer() -> std::shared_ptr<wayland_rs::WlPointerImpl>
{
    auto const pointer = std::make_shared<WlPointer>(client);
    auto dispatcher = std::make_shared<PointerEventDispatcher>(pointer);

    seat->pointer_listeners->register_listener(client.get(), dispatcher.get());
    pointer->add_destroy_listener(
        [listeners = seat->pointer_listeners, listener = std::move(dispatcher), client = client.get()]()
        {
            listeners->unregister_listener(client, listener.get());
        });
    return pointer;
}

auto mf::WlSeatGlobal::Instance::get_keyboard() -> std::shared_ptr<wayland_rs::WlKeyboardImpl>
{
    auto const keyboard = std::make_shared<WlKeyboard>(*seat, client);

    seat->keyboard_listeners->register_listener(client.get(), keyboard.get());
    keyboard->add_destroy_listener(
        [listeners = seat->keyboard_listeners, listener = keyboard, client = client.get()]()
        {
            listeners->unregister_listener(client, listener.get());
        });
    return keyboard;
}

auto mf::WlSeatGlobal::Instance::get_touch() -> std::shared_ptr<wayland_rs::WlTouchImpl>
{
    auto const touch = std::make_shared<WlTouch>(seat->clock, client);
    seat->touch_listeners->register_listener(client.get(), touch.get());
    touch->add_destroy_listener(
        [listeners = seat->touch_listeners, listener = touch, client = client.get()]()
        {
            listeners->unregister_listener(client, listener.get());
        });
    return touch;
}

void mf::WlSeatGlobal::add_focus_listener(mw::Client* client, FocusListener* listener)
{
    focus_listeners->register_listener(client, listener);
    if (focused_client == client)
    {
        if (focused_surface)
            listener->focus_on(focused_surface.value().shared_from_this());
        else
            listener->focus_on(nullptr);
    }
    else
    {
        listener->focus_on(nullptr);
    }
}

void mf::WlSeatGlobal::remove_focus_listener(mw::Client* client, FocusListener* listener)
{
    focus_listeners->unregister_listener(client, listener);
}

mf::PointerEventDispatcher::PointerEventDispatcher(std::shared_ptr<WlPointer> const& wl_pointer) :
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
    this->wl_data_device = mw::Weak<WlDataDevice>{wl_data_device->shared_from_this()};

    if (wl_pointer)
    {
        wl_pointer.value().leave(std::nullopt);
    }
}

void mf::PointerEventDispatcher::stop_dispatch_to_data_device()
{
    wl_data_device = mw::Weak<WlDataDevice>{};
}
