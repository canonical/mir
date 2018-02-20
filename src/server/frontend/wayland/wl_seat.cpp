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
 *              William Wold <william.wold@canonical.com>
 */

#include "core_generated_interfaces.h"

#include "wl_seat.h"

#include "wayland_utils.h"
#include "wl_surface.h"

#include "mir/executor.h"
#include "mir/client/event.h"
#include "mir/anonymous_shm_file.h"

#include "mir/input/input_device_observer.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/seat.h"
#include "mir/input/device.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/input/keymap.h"

#include <linux/input-event-codes.h>

#include <cstring> // memcpy
#include <mutex>
#include <unordered_set>

namespace mir
{
namespace frontend
{
class WlPointer;
class WlTouch;

class WlKeyboard : public wayland::Keyboard
{
public:
    WlKeyboard(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        mir::input::Keymap const& initial_keymap,
        std::function<void(WlKeyboard*)> const& on_destroy,
        std::function<std::vector<uint32_t>()> const& acquire_current_keyboard_state,
        std::shared_ptr<mir::Executor> const& executor)
        : Keyboard(client, parent, id),
          keymap{nullptr, &xkb_keymap_unref},
          state{nullptr, &xkb_state_unref},
          context{xkb_context_new(XKB_CONTEXT_NO_FLAGS), &xkb_context_unref},
          executor{executor},
          on_destroy{on_destroy},
          acquire_current_keyboard_state{acquire_current_keyboard_state},
          destroyed{std::make_shared<bool>(false)}
    {
        // TODO: We should really grab the keymap for the focused surface when
        // we receive focus.

        // TODO: Maintain per-device keymaps, and send the appropriate map before
        // sending an event from a keyboard with a different map.

        /* The wayland::Keyboard constructor has already run, creating the keyboard
         * resource. It is thus safe to send a keymap event to it; the client will receive
         * the keyboard object before this event.
         */
        set_keymap(initial_keymap);
    }

    ~WlKeyboard()
    {
        on_destroy(this);
        *destroyed = true;
    }

    void handle_event(MirInputEvent const* event, wl_resource* /*target*/)
    {
        executor->spawn(run_unless(
            destroyed,
            [
                ev = mir::client::Event{mir_event_ref(mir_input_event_get_event(event))},
                this
            ] ()
            {
                auto const serial = wl_display_next_serial(wl_client_get_display(client));
                auto const event = mir_event_get_input_event(ev);
                auto const key_event = mir_input_event_get_keyboard_event(event);
                auto const scancode = mir_keyboard_event_scan_code(key_event);
                /*
                 * HACK! Maintain our own XKB state, so we can serialise it for
                 * wl_keyboard_send_modifiers
                 */

                switch (mir_keyboard_event_action(key_event))
                {
                    case mir_keyboard_action_up:
                        xkb_state_update_key(state.get(), scancode + 8, XKB_KEY_UP);
                        wl_keyboard_send_key(resource,
                            serial,
                            mir_input_event_get_event_time(event) / 1000,
                            mir_keyboard_event_scan_code(key_event),
                            WL_KEYBOARD_KEY_STATE_RELEASED);
                        break;
                    case mir_keyboard_action_down:
                        xkb_state_update_key(state.get(), scancode + 8, XKB_KEY_DOWN);
                        wl_keyboard_send_key(resource,
                            serial,
                            mir_input_event_get_event_time(event) / 1000,
                            mir_keyboard_event_scan_code(key_event),
                            WL_KEYBOARD_KEY_STATE_PRESSED);
                        break;
                    default:
                        break;
                }
                update_modifier_state();
            }));
    }

    void handle_event(MirWindowEvent const* event, wl_resource* target)
    {
        if (mir_window_event_get_attribute(event) == mir_window_attrib_focus)
        {
            executor->spawn(run_unless(
                destroyed,
                [
                    target = target,
                    target_window_destroyed = WlSurface::from(target)->destroyed_flag(),
                    focussed = mir_window_event_get_attribute_value(event),
                    this
                ]()
                {
                    if (*target_window_destroyed)
                        return;

                    auto const serial = wl_display_next_serial(wl_client_get_display(client));
                    if (focussed)
                    {
                        /*
                         * TODO:
                         *  *) Send the surface's keymap here.
                         */
                        auto const keyboard_state = acquire_current_keyboard_state();

                        wl_array key_state;
                        wl_array_init(&key_state);

                        auto* const array_storage = wl_array_add(
                            &key_state,
                            keyboard_state.size() * sizeof(decltype(keyboard_state)::value_type));
                        if (!array_storage)
                        {
                            wl_resource_post_no_memory(resource);
                            BOOST_THROW_EXCEPTION(std::bad_alloc());
                        }
                        std::memcpy(
                            array_storage,
                            keyboard_state.data(),
                            keyboard_state.size() * sizeof(decltype(keyboard_state)::value_type));

                        // Rebuild xkb state
                        state = decltype(state)(xkb_state_new(keymap.get()), &xkb_state_unref);
                        for (auto scancode : keyboard_state)
                        {
                            xkb_state_update_key(state.get(), scancode + 8, XKB_KEY_DOWN);
                        }
                        update_modifier_state();

                        wl_keyboard_send_enter(resource, serial, target, &key_state);
                        wl_array_release(&key_state);
                    }
                    else
                    {
                        wl_keyboard_send_leave(resource, serial, target);
                    }
                }));
        }
    }

    void handle_event(MirKeymapEvent const* event, wl_resource* /*target*/)
    {
        char const* buffer;
        size_t length;

        mir_keymap_event_get_keymap_buffer(event, &buffer, &length);

        mir::AnonymousShmFile shm_buffer{length};
        memcpy(shm_buffer.base_ptr(), buffer, length);

        wl_keyboard_send_keymap(
            resource,
            WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
            shm_buffer.fd(),
            length);

        keymap = decltype(keymap)(xkb_keymap_new_from_buffer(
            context.get(),
            buffer,
            length,
            XKB_KEYMAP_FORMAT_TEXT_V1,
            XKB_KEYMAP_COMPILE_NO_FLAGS),
            &xkb_keymap_unref);

        state = decltype(state)(xkb_state_new(keymap.get()), &xkb_state_unref);
    }

    void set_keymap(mir::input::Keymap const& new_keymap)
    {
        xkb_rule_names const names = {
            "evdev",
            new_keymap.model.c_str(),
            new_keymap.layout.c_str(),
            new_keymap.variant.c_str(),
            new_keymap.options.c_str()
        };
        keymap = decltype(keymap){
            xkb_keymap_new_from_names(
                context.get(),
                &names,
                XKB_KEYMAP_COMPILE_NO_FLAGS),
            &xkb_keymap_unref};

        // TODO: We might need to copy across the existing depressed keys?
        state = decltype(state)(xkb_state_new(keymap.get()), &xkb_state_unref);

        auto buffer = xkb_keymap_get_as_string(keymap.get(), XKB_KEYMAP_FORMAT_TEXT_V1);
        auto length = strlen(buffer);

        mir::AnonymousShmFile shm_buffer{length};
        memcpy(shm_buffer.base_ptr(), buffer, length);

        wl_keyboard_send_keymap(
            resource,
            WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
            shm_buffer.fd(),
            length);
    }

private:
    void update_modifier_state()
    {
        // TODO?
        // assert_on_wayland_event_loop()

        auto new_depressed_mods = xkb_state_serialize_mods(
            state.get(),
            XKB_STATE_MODS_DEPRESSED);
        auto new_latched_mods = xkb_state_serialize_mods(
            state.get(),
            XKB_STATE_MODS_LATCHED);
        auto new_locked_mods = xkb_state_serialize_mods(
            state.get(),
            XKB_STATE_MODS_LOCKED);
        auto new_group = xkb_state_serialize_layout(
            state.get(),
            XKB_STATE_LAYOUT_EFFECTIVE);

        if ((new_depressed_mods != mods_depressed) ||
            (new_latched_mods != mods_latched) ||
            (new_locked_mods != mods_locked) ||
            (new_group != group))
        {
            mods_depressed = new_depressed_mods;
            mods_latched = new_latched_mods;
            mods_locked = new_locked_mods;
            group = new_group;

            wl_keyboard_send_modifiers(
                resource,
                wl_display_get_serial(wl_client_get_display(client)),
                mods_depressed,
                mods_latched,
                mods_locked,
                group);
        }
    }

    std::unique_ptr<xkb_keymap, decltype(&xkb_keymap_unref)> keymap;
    std::unique_ptr<xkb_state, decltype(&xkb_state_unref)> state;
    std::unique_ptr<xkb_context, decltype(&xkb_context_unref)> const context;

    std::shared_ptr<mir::Executor> const executor;
    std::function<void(WlKeyboard*)> on_destroy;
    std::function<std::vector<uint32_t>()> const acquire_current_keyboard_state;
    std::shared_ptr<bool> const destroyed;

    uint32_t mods_depressed{0};
    uint32_t mods_latched{0};
    uint32_t mods_locked{0};
    uint32_t group{0};

    void release() override;
};

void WlKeyboard::release()
{
    wl_resource_destroy(resource);
}

class WlPointer : public wayland::Pointer
{
public:

    WlPointer(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        std::function<void(WlPointer*)> const& on_destroy,
        std::shared_ptr<mir::Executor> const& executor)
        : Pointer(client, parent, id),
          display{wl_client_get_display(client)},
          executor{executor},
          on_destroy{on_destroy},
          destroyed{std::make_shared<bool>(false)}
    {
    }

    ~WlPointer()
    {
        *destroyed = true;
        on_destroy(this);
    }

    void handle_event(MirInputEvent const* event, wl_resource* target)
    {
        executor->spawn(run_unless(
            destroyed,
            [
                ev = mir::client::Event{mir_input_event_get_event(event)},
                target,
                target_window_destroyed = WlSurface::from(target)->destroyed_flag(),
                this
            ]()
            {
                if (*target_window_destroyed)
                    return;

                auto const serial = wl_display_next_serial(display);
                auto const event = mir_event_get_input_event(ev);
                auto const pointer_event = mir_input_event_get_pointer_event(event);

                switch(mir_pointer_event_action(pointer_event))
                {
                    case mir_pointer_action_button_down:
                    case mir_pointer_action_button_up:
                    {
                        auto const current_set  = mir_pointer_event_buttons(pointer_event);
                        auto const current_time = mir_input_event_get_event_time(event) / 1000;

                        for (auto const& mapping :
                            {
                                std::make_pair(mir_pointer_button_primary, BTN_LEFT),
                                std::make_pair(mir_pointer_button_secondary, BTN_RIGHT),
                                std::make_pair(mir_pointer_button_tertiary, BTN_MIDDLE),
                                std::make_pair(mir_pointer_button_back, BTN_BACK),
                                std::make_pair(mir_pointer_button_forward, BTN_FORWARD),
                                std::make_pair(mir_pointer_button_side, BTN_SIDE),
                                std::make_pair(mir_pointer_button_task, BTN_TASK),
                                std::make_pair(mir_pointer_button_extra, BTN_EXTRA)
                            })
                        {
                            if (mapping.first & (current_set ^ last_set))
                            {
                                auto const action = (mapping.first & current_set) ?
                                                    WL_POINTER_BUTTON_STATE_PRESSED :
                                                    WL_POINTER_BUTTON_STATE_RELEASED;

                                wl_pointer_send_button(resource, serial, current_time, mapping.second, action);
                            }
                        }

                        last_set = current_set;
                        break;
                    }
                    case mir_pointer_action_enter:
                    {
                        wl_pointer_send_enter(
                            resource,
                            serial,
                            target,
                            wl_fixed_from_double(mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)),
                            wl_fixed_from_double(mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)));
                        break;
                    }
                    case mir_pointer_action_leave:
                    {
                        wl_pointer_send_leave(
                            resource,
                            serial,
                            target);
                        break;
                    }
                    case mir_pointer_action_motion:
                    {
                        auto x = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x);
                        auto y = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y);
                        auto vscroll = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll);
                        auto hscroll = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll);

                        if ((x != last_x) || (y != last_y))
                        {
                            wl_pointer_send_motion(
                                resource,
                                mir_input_event_get_event_time(event) / 1000,
                                wl_fixed_from_double(x),
                                wl_fixed_from_double(y));

                            last_x = x;
                            last_y = y;
                        }
                        if (vscroll != last_vscroll)
                        {
                            wl_pointer_send_axis(
                                resource,
                                mir_input_event_get_event_time(event) / 1000,
                                WL_POINTER_AXIS_VERTICAL_SCROLL,
                                wl_fixed_from_double(vscroll));
                            last_vscroll = vscroll;
                        }
                        if (hscroll != last_hscroll)
                        {
                            wl_pointer_send_axis(
                                resource,
                                mir_input_event_get_event_time(event) / 1000,
                                WL_POINTER_AXIS_HORIZONTAL_SCROLL,
                                wl_fixed_from_double(hscroll));
                            last_hscroll = hscroll;
                        }
                        break;
                    }
                    case mir_pointer_actions:
                        break;
                }
            }));
    }

    // Pointer interface
private:
    wl_display* const display;
    std::shared_ptr<mir::Executor> const executor;

    std::function<void(WlPointer*)> on_destroy;
    std::shared_ptr<bool> const destroyed;

    MirPointerButtons last_set{0};
    float last_x{0}, last_y{0}, last_vscroll{0}, last_hscroll{0};

    void set_cursor(uint32_t serial, std::experimental::optional<wl_resource*> const& surface, int32_t hotspot_x, int32_t hotspot_y) override;
    void release() override;
};

void WlPointer::set_cursor(uint32_t serial, std::experimental::optional<wl_resource*> const& surface, int32_t hotspot_x, int32_t hotspot_y)
{
    (void)serial;
    (void)surface;
    (void)hotspot_x;
    (void)hotspot_y;
}

void WlPointer::release()
{
    wl_resource_destroy(resource);
}

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
