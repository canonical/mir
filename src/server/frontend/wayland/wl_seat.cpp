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

#include "core_generated_interfaces.h"

#include "wl_seat.h"

#include "wayland_utils.h"
#include "wl_surface.h"
#include "wl_keybaord.h"
#include "wl_pointer.h"

#include "mir/executor.h"
#include "mir/client/event.h"

#include "mir/input/input_device_observer.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/seat.h"
#include "mir/input/device.h"
#include "mir/input/keymap.h"
#include "mir/input/mir_keyboard_config.h"

#include <mutex>
#include <unordered_set>

namespace mir
{
class Executor;

namespace frontend
{

class WlTouch : public wayland::Touch
{
public:
    WlTouch(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        std::function<void(WlTouch*)> const& on_destroy,
        std::shared_ptr<mir::Executor> const& executor)
        : Touch(client, parent, id),
          executor{executor},
          on_destroy{on_destroy},
          destroyed{std::make_shared<bool>(false)}
    {
    }

    ~WlTouch()
    {
        on_destroy(this);
        *destroyed = true;
    }

    void handle_event(MirInputEvent const* event, wl_resource* target)
    {
        executor->spawn(run_unless(
            destroyed,
            [
                ev = mir::client::Event{mir_input_event_get_event(event)},
                target = target,
                target_window_destroyed = WlSurface::from(target)->destroyed_flag(),
                this
            ]()
            {
                if (*target_window_destroyed)
                    return;

                auto const input_ev = mir_event_get_input_event(ev);
                auto const touch_ev = mir_input_event_get_touch_event(input_ev);

                for (auto i = 0u; i < mir_touch_event_point_count(touch_ev); ++i)
                {
                    auto const touch_id = mir_touch_event_id(touch_ev, i);
                    auto const action = mir_touch_event_action(touch_ev, i);
                    auto const x = mir_touch_event_axis_value(
                        touch_ev,
                        i,
                        mir_touch_axis_x);
                    auto const y = mir_touch_event_axis_value(
                        touch_ev,
                        i,
                        mir_touch_axis_y);

                    switch (action)
                    {
                    case mir_touch_action_down:
                        wl_touch_send_down(
                            resource,
                            wl_display_get_serial(wl_client_get_display(client)),
                            mir_input_event_get_event_time(input_ev) / 1000,
                            target,
                            touch_id,
                            wl_fixed_from_double(x),
                            wl_fixed_from_double(y));
                        break;
                    case mir_touch_action_up:
                        wl_touch_send_up(
                            resource,
                            wl_display_get_serial(wl_client_get_display(client)),
                            mir_input_event_get_event_time(input_ev) / 1000,
                            touch_id);
                        break;
                    case mir_touch_action_change:
                        wl_touch_send_motion(
                            resource,
                            mir_input_event_get_event_time(input_ev) / 1000,
                            touch_id,
                            wl_fixed_from_double(x),
                            wl_fixed_from_double(y));
                        break;
                    case mir_touch_actions:
                        /*
                         * We should never receive an event with this action set;
                         * the only way would be if a *new* action has been added
                         * to the enum, and this hasn't been updated.
                         *
                         * There's nothing to do here, but don't use default: so
                         * that the compiler will warn if a new enum value is added.
                         */
                        break;
                    }
                }

                if (mir_touch_event_point_count(touch_ev) > 0)
                {
                    /*
                     * This is mostly paranoia; I assume we won't actually be called
                     * with an empty touch event.
                     *
                     * Regardless, the Wayland protocol requires that there be at least
                     * one event sent before we send the ending frame, so make that explicit.
                     */
                    wl_touch_send_frame(resource);
                }
            }
            ));
    }

    // Touch interface
private:
    std::shared_ptr<mir::Executor> const executor;
    std::function<void(WlTouch*)> on_destroy;
    std::shared_ptr<bool> const destroyed;

    void release() override;
};

void WlTouch::release()
{
    wl_resource_destroy(resource);
}

template<class InputInterface>
class InputCtx
{
public:
    InputCtx() = default;

    InputCtx(InputCtx&&) = delete;
    InputCtx(InputCtx const&) = delete;
    InputCtx& operator=(InputCtx const&) = delete;

    void register_listener(InputInterface* listener)
    {
        std::lock_guard<std::mutex> lock{mutex};
        listeners.push_back(listener);
    }

    void unregister_listener(InputInterface const* listener)
    {
        std::lock_guard<std::mutex> lock{mutex};
        listeners.erase(
            std::remove(
                listeners.begin(),
                listeners.end(),
                listener),
            listeners.end());
    }

    void handle_event(MirInputEvent const* event, wl_resource* target) const
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto listener : listeners)
        {
            listener->handle_event(event, target);
        }
    }

    void handle_event(MirWindowEvent const* event, wl_resource* target) const
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto& listener : listeners)
        {
            listener->handle_event(event, target);
        }
    }

    void handle_event(MirKeymapEvent const* event, wl_resource* target) const
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto& listener : listeners)
        {
            listener->handle_event(event, target);
        }
    }

private:
    std::mutex mutable mutex;
    std::vector<InputInterface*> listeners;
};

class WlSeat::ConfigObserver : public mir::input::InputDeviceObserver
{
public:
    ConfigObserver(
        mir::input::Keymap const& keymap,
        std::function<void(mir::input::Keymap const&)> const& on_keymap_commit)
        : current_keymap{keymap},
            on_keymap_commit{on_keymap_commit}
    {
    }

    void device_added(std::shared_ptr<input::Device> const& device) override;
    void device_changed(std::shared_ptr<input::Device> const& device) override;
    void device_removed(std::shared_ptr<input::Device> const& device) override;
    void changes_complete() override;

private:
    mir::input::Keymap const& current_keymap;
    mir::input::Keymap pending_keymap;
    std::function<void(mir::input::Keymap const&)> const on_keymap_commit;
};

void WlSeat::ConfigObserver::device_added(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        if (current_keymap != keyboard_config.value().device_keymap())
        {
            pending_keymap = keyboard_config.value().device_keymap();
        }
    }
}

void WlSeat::ConfigObserver::device_changed(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        if (current_keymap != keyboard_config.value().device_keymap())
        {
            pending_keymap = keyboard_config.value().device_keymap();
        }
    }
}

void WlSeat::ConfigObserver::device_removed(std::shared_ptr<input::Device> const& /*device*/)
{
}

void WlSeat::ConfigObserver::changes_complete()
{
    on_keymap_commit(pending_keymap);
}

struct wl_seat_interface const WlSeat::vtable = {
    WlSeat::get_pointer,
    WlSeat::get_keyboard,
    WlSeat::get_touch,
    WlSeat::release
};

WlSeat::WlSeat(
    wl_display* display,
    std::shared_ptr<mir::input::InputDeviceHub> const& input_hub,
    std::shared_ptr<mir::input::Seat> const& seat,
    std::shared_ptr<mir::Executor> const& executor)
    :   keymap{std::make_unique<input::Keymap>()},
        config_observer{
            std::make_shared<ConfigObserver>(
                *keymap,
                [this](mir::input::Keymap const& new_keymap)
                {
                    *keymap = new_keymap;
                })},
        pointer{std::make_unique<std::unordered_map<wl_client*, InputCtx<WlPointer>>>()},
        keyboard{std::make_unique<std::unordered_map<wl_client*, InputCtx<WlKeyboard>>>()},
        touch{std::make_unique<std::unordered_map<wl_client*, InputCtx<WlTouch>>>()},
        input_hub{input_hub},
        seat{seat},
        executor{executor},
        global{wl_global_create(
            display,
            &wl_seat_interface,
            5,
            this,
            &WlSeat::bind)}
{
    if (!global)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to export wl_seat interface"));
    }
    input_hub->add_observer(config_observer);
}

WlSeat::~WlSeat()
{
    input_hub->remove_observer(config_observer);
    wl_global_destroy(global);
}

void WlSeat::handle_pointer_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    (*pointer)[client].handle_event(input_event, target);
}

void WlSeat::handle_keyboard_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    (*keyboard)[client].handle_event(input_event, target);
}

void WlSeat::handle_touch_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    (*touch)[client].handle_event(input_event, target);
}

void WlSeat::handle_event(wl_client* client, MirKeymapEvent const* keymap_event, wl_resource* target) const
{
    (*keyboard)[client].handle_event(keymap_event, target);
}

void WlSeat::handle_event(wl_client* client, MirWindowEvent const* window_event, wl_resource* target) const
{
    (*keyboard)[client].handle_event(window_event, target);
}

void WlSeat::spawn(std::function<void()>&& work)
{
    executor->spawn(std::move(work));
}

void WlSeat::bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
{
    auto me = reinterpret_cast<WlSeat*>(data);
    auto resource = wl_resource_create(client, &wl_seat_interface,
        std::min(version, 6u), id);
    if (resource == nullptr)
    {
        wl_client_post_no_memory(client);
        BOOST_THROW_EXCEPTION((std::bad_alloc{}));
    }
    wl_resource_set_implementation(resource, &vtable, me, nullptr);

    /*
        * TODO: Read the actual capabilities. Do we have a keyboard? Mouse? Touch?
        */
    if (version >= WL_SEAT_CAPABILITIES_SINCE_VERSION)
    {
        wl_seat_send_capabilities(
            resource,
            WL_SEAT_CAPABILITY_POINTER |
            WL_SEAT_CAPABILITY_KEYBOARD |
            WL_SEAT_CAPABILITY_TOUCH);
    }
    if (version >= WL_SEAT_NAME_SINCE_VERSION)
    {
        wl_seat_send_name(
            resource,
            "seat0");
    }

    wl_resource_set_user_data(resource, me);
}

void WlSeat::get_pointer(wl_client* client, wl_resource* resource, uint32_t id)
{
    auto me = reinterpret_cast<WlSeat*>(wl_resource_get_user_data(resource));
    auto& input_ctx = (*me->pointer)[client];
    input_ctx.register_listener(
        new WlPointer{
            client,
            resource,
            id,
            [&input_ctx](WlPointer* listener)
            {
                input_ctx.unregister_listener(listener);
            },
            me->executor});
}

void WlSeat::get_keyboard(wl_client* client, wl_resource* resource, uint32_t id)
{
    auto me = reinterpret_cast<WlSeat*>(wl_resource_get_user_data(resource));
    auto& input_ctx = (*me->keyboard)[client];

    input_ctx.register_listener(
        new WlKeyboard{
            client,
            resource,
            id,
            *me->keymap,
            [&input_ctx](WlKeyboard* listener)
            {
                input_ctx.unregister_listener(listener);
            },
            [me]()
            {
                std::unordered_set<uint32_t> pressed_keys;

                auto const ev = me->seat->create_device_state();
                auto const state_event = mir_event_get_input_device_state_event(ev.get());
                for (
                    auto dev = 0u;
                    dev < mir_input_device_state_event_device_count(state_event);
                    ++dev)
                {
                    for (
                        auto idx = 0u;
                        idx < mir_input_device_state_event_device_pressed_keys_count(state_event, dev);
                        ++idx)
                    {
                        pressed_keys.insert(
                            mir_input_device_state_event_device_pressed_keys_for_index(
                                state_event,
                                dev,
                                idx));
                    }
                }

                return std::vector<uint32_t>{pressed_keys.begin(), pressed_keys.end()};
            },
            me->executor});
}

void WlSeat::get_touch(wl_client* client, wl_resource* resource, uint32_t id)
{
    auto me = reinterpret_cast<WlSeat*>(wl_resource_get_user_data(resource));
    auto& input_ctx = (*me->touch)[client];

    input_ctx.register_listener(
        new WlTouch{
            client,
            resource,
            id,
            [&input_ctx](WlTouch* listener)
            {
                input_ctx.unregister_listener(listener);
            },
            me->executor});
}

void WlSeat::release(struct wl_client* /*client*/, struct wl_resource* us)
{
    wl_resource_destroy(us);
}
}
}
