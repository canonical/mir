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
#include "wl_keyboard.h"
#include "wl_pointer.h"
#include "wl_touch.h"

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

namespace mf = mir::frontend;
namespace mi = mir::input;

namespace mir
{
class Executor;
}

template<class InputInterface>
class mf::InputCtx
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

class mf::WlSeat::ConfigObserver : public mi::InputDeviceObserver
{
public:
    ConfigObserver(
        mi::Keymap const& keymap,
        std::function<void(mi::Keymap const&)> const& on_keymap_commit)
        : current_keymap{keymap},
            on_keymap_commit{on_keymap_commit}
    {
    }

    void device_added(std::shared_ptr<input::Device> const& device) override;
    void device_changed(std::shared_ptr<input::Device> const& device) override;
    void device_removed(std::shared_ptr<input::Device> const& device) override;
    void changes_complete() override;

private:
    mi::Keymap const& current_keymap;
    mi::Keymap pending_keymap;
    std::function<void(mi::Keymap const&)> const on_keymap_commit;
};

void mf::WlSeat::ConfigObserver::device_added(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        if (current_keymap != keyboard_config.value().device_keymap())
        {
            pending_keymap = keyboard_config.value().device_keymap();
        }
    }
}

void mf::WlSeat::ConfigObserver::device_changed(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        if (current_keymap != keyboard_config.value().device_keymap())
        {
            pending_keymap = keyboard_config.value().device_keymap();
        }
    }
}

void mf::WlSeat::ConfigObserver::device_removed(std::shared_ptr<input::Device> const& /*device*/)
{
}

void mf::WlSeat::ConfigObserver::changes_complete()
{
    on_keymap_commit(pending_keymap);
}

mf::WlSeat::WlSeat(
    wl_display* display,
    std::shared_ptr<mi::InputDeviceHub> const& input_hub,
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<mir::Executor> const& executor)
    :   Seat(display, 5),
        keymap{std::make_unique<input::Keymap>()},
        config_observer{
            std::make_shared<ConfigObserver>(
                *keymap,
                [this](mi::Keymap const& new_keymap)
                {
                    *keymap = new_keymap;
                })},
        pointer{std::make_unique<std::unordered_map<wl_client*, InputCtx<WlPointer>>>()},
        keyboard{std::make_unique<std::unordered_map<wl_client*, InputCtx<WlKeyboard>>>()},
        touch{std::make_unique<std::unordered_map<wl_client*, InputCtx<WlTouch>>>()},
        input_hub{input_hub},
        seat{seat},
        executor{executor}
{
    input_hub->add_observer(config_observer);
}

mf::WlSeat::~WlSeat()
{
    input_hub->remove_observer(config_observer);
}

void mf::WlSeat::handle_pointer_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    (*pointer)[client].handle_event(input_event, target);
}

void mf::WlSeat::handle_keyboard_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    (*keyboard)[client].handle_event(input_event, target);
}

void mf::WlSeat::handle_touch_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    (*touch)[client].handle_event(input_event, target);
}

void mf::WlSeat::handle_event(wl_client* client, MirKeymapEvent const* keymap_event, wl_resource* target) const
{
    (*keyboard)[client].handle_event(keymap_event, target);
}

void mf::WlSeat::handle_event(wl_client* client, MirWindowEvent const* window_event, wl_resource* target) const
{
    (*keyboard)[client].handle_event(window_event, target);
}

void mf::WlSeat::spawn(std::function<void()>&& work)
{
    executor->spawn(std::move(work));
}

void mf::WlSeat::bind(wl_client* /*client*/, wl_resource* resource)
{
    // TODO: Read the actual capabilities. Do we have a keyboard? Mouse? Touch?
    int version = wl_resource_get_version(resource);
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
        wl_seat_send_name(resource, "seat0");
    }
}

void mf::WlSeat::get_pointer(wl_client* client, wl_resource* resource, uint32_t id)
{
    auto& input_ctx = (*pointer)[client];
    input_ctx.register_listener(
        new WlPointer{
            client,
            resource,
            id,
            [&input_ctx](WlPointer* listener)
            {
                input_ctx.unregister_listener(listener);
            },
            executor});
}

void mf::WlSeat::get_keyboard(wl_client* client, wl_resource* resource, uint32_t id)
{
    auto& input_ctx = (*keyboard)[client];

    input_ctx.register_listener(
        new WlKeyboard{
            client,
            resource,
            id,
            *keymap,
            [&input_ctx](WlKeyboard* listener)
            {
                input_ctx.unregister_listener(listener);
            },
            [this]()
            {
                std::unordered_set<uint32_t> pressed_keys;

                auto const ev = seat->create_device_state();
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
            executor});
}

void mf::WlSeat::get_touch(wl_client* client, wl_resource* resource, uint32_t id)
{
    auto& input_ctx = (*touch)[client];

    input_ctx.register_listener(
        new WlTouch{
            client,
            resource,
            id,
            [&input_ctx](WlTouch* listener)
            {
                input_ctx.unregister_listener(listener);
            },
            executor});
}

void mf::WlSeat::release(struct wl_client* /*client*/, struct wl_resource* us)
{
    wl_resource_destroy(us);
}
