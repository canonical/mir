/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/input/xkb_mapper_registrar.h"
#include "mir/input/keymap.h"
#include "mir/input/keyboard_leds.h"
#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"

#include <linux/input-event-codes.h>

#include <unordered_set>

namespace mi = mir::input;
namespace mev = mir::events;
namespace mircv = mi::receiver;

namespace
{

char const* get_locale_from_environment()
{
    char const* loc = getenv("LC_ALL");
    if (!loc)
        loc = getenv("LC_CTYPE");
    if (!loc)
        loc = getenv("LANG");
    if (!loc)
        loc = "C";
    return loc;
}

uint32_t constexpr to_xkb_scan_code(uint32_t evdev_scan_code)
{
    // xkb scancodes are offset by 8 from evdev scancodes for compatibility with X protocol.
    return evdev_scan_code + 8;
}

MirInputEventModifiers modifier_from_xkb_scan_code(uint32_t key)
{
    switch(key)
    {
    case to_xkb_scan_code(KEY_RIGHTSHIFT): return mir_input_event_modifier_shift_right;
    case to_xkb_scan_code(KEY_LEFTSHIFT): return mir_input_event_modifier_shift_left;
    case to_xkb_scan_code(KEY_RIGHTALT): return mir_input_event_modifier_alt_right;
    case to_xkb_scan_code(KEY_LEFTALT): return mir_input_event_modifier_alt_left;
    case to_xkb_scan_code(KEY_RIGHTCTRL): return mir_input_event_modifier_ctrl_right;
    case to_xkb_scan_code(KEY_LEFTCTRL): return mir_input_event_modifier_ctrl_left;
    case to_xkb_scan_code(KEY_LEFTMETA): return mir_input_event_modifier_meta_left;
    case to_xkb_scan_code(KEY_RIGHTMETA): return mir_input_event_modifier_meta_right;
    case to_xkb_scan_code(KEY_CAPSLOCK): return mir_input_event_modifier_caps_lock;
    case to_xkb_scan_code(KEY_SCREENLOCK): return mir_input_event_modifier_scroll_lock;
    case to_xkb_scan_code(KEY_NUMLOCK): return mir_input_event_modifier_num_lock;
    default: return MirInputEventModifiers{0};
    }
}

bool is_toggle_modifier(MirInputEventModifiers key)
{
    return key == mir_input_event_modifier_caps_lock ||
        key == mir_input_event_modifier_scroll_lock ||
        key == mir_input_event_modifier_num_lock;
}

MirInputEventModifiers expand_modifiers(MirInputEventModifiers modifiers)
{
    if (modifiers == 0)
        return mir_input_event_modifier_none;

    if ((modifiers & mir_input_event_modifier_alt_left) || (modifiers & mir_input_event_modifier_alt_right))
        modifiers |= mir_input_event_modifier_alt;

    if ((modifiers & mir_input_event_modifier_ctrl_left) || (modifiers & mir_input_event_modifier_ctrl_right))
        modifiers |= mir_input_event_modifier_ctrl;

    if ((modifiers & mir_input_event_modifier_shift_left) || (modifiers & mir_input_event_modifier_shift_right))
        modifiers |= mir_input_event_modifier_shift;

    if ((modifiers & mir_input_event_modifier_meta_left) || (modifiers & mir_input_event_modifier_meta_right))
        modifiers |= mir_input_event_modifier_meta;

    return modifiers;
}

mi::XKBComposeStatePtr make_unique_compose_state(mi::XKBComposeTablePtr const& table)
{
    return {xkb_compose_state_new(table.get(), XKB_COMPOSE_STATE_NO_FLAGS), &xkb_compose_state_unref};
}

mi::XKBComposeTablePtr make_unique_compose_table_from_locale(mi::XKBContextPtr const& context, std::string const& locale)
{
    return {xkb_compose_table_new_from_locale(
            context.get(),
            locale.c_str(),
            XKB_COMPOSE_COMPILE_NO_FLAGS),
           &xkb_compose_table_unref};
}

mi::XKBStatePtr make_unique_state(xkb_keymap* keymap)
{
    return {xkb_state_new(keymap), xkb_state_unref};
}
}

mi::XKBContextPtr mi::make_unique_context()
{
    auto context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!context)
    {
        fatal_error("Failed to create XKB context");
    }
    return {context, &xkb_context_unref};
}

mircv::XKBMapperRegistrar::XkbMappingState::XKBMappingStateRegistrar::XKBMappingStateRegistrar(mir::Executor& executor)
    : ObserverMultiplexer<LedObserver>(executor)
{
}

void mircv::XKBMapperRegistrar::XkbMappingState::XKBMappingStateRegistrar::leds_set(mir::input::KeyboardLeds leds)
{
    for_each_observer(&LedObserver::leds_set, leds);
}

mircv::XKBMapperRegistrar::XKBMapperRegistrar(Executor& executor) :
    executor(executor),
    context{make_unique_context()},
    compose_table{make_unique_compose_table_from_locale(context, get_locale_from_environment())}
{
}

void mircv::XKBMapperRegistrar::set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& key_state)
{
    std::lock_guard lg(guard);

    auto mapping_state = get_keymapping_state(id);
    if (mapping_state)
        mapping_state->set_key_state(key_state);
}

void mircv::XKBMapperRegistrar::update_modifier()
{
    modifier_state = mir::optional_value<MirInputEventModifiers>{};
    xkb_modifiers_ = {};
    if (!device_mapping.empty())
    {
        MirInputEventModifiers new_modifier = 0;
        for (auto const& mapping_state : device_mapping)
        {
            new_modifier |= mapping_state.second->modifiers();
            auto const device_xkb_modifiers = mapping_state.second->xkb_modifiers();
            xkb_modifiers_.depressed |= device_xkb_modifiers.depressed;
            xkb_modifiers_.latched |= device_xkb_modifiers.latched;
            xkb_modifiers_.locked |= device_xkb_modifiers.locked;
            if (last_device_id && mapping_state.first == last_device_id.value())
            {
                xkb_modifiers_.effective_layout = device_xkb_modifiers.effective_layout;
            }
        }

        modifier_state = new_modifier;
    }
}

void mircv::XKBMapperRegistrar::map_event(MirEvent& ev)
{
    std::lock_guard lg(guard);

    auto type = mir_event_get_type(&ev);

    if (type == mir_event_type_input)
    {
        auto input_event = mir_event_get_input_event(&ev);
        auto input_type = mir_input_event_get_type(input_event);
        auto device_id = mir_input_event_get_device_id(input_event);

        if (input_type == mir_input_event_type_key)
        {
            auto mapping_state = get_keymapping_state(device_id);
            if (mapping_state)
            {
                last_device_id = device_id;
                auto compose_state = get_compose_state(device_id);
                if (mapping_state->update_and_map(ev, compose_state))
                    update_modifier();
            }

            auto& key_event = *ev.to_input()->to_keyboard();
            if (!key_event.xkb_modifiers())
            {
                key_event.set_xkb_modifiers(xkb_modifiers_);
            }
        }
        else if (modifier_state.is_set())
        {
            mev::set_modifier(ev, expand_modifiers(modifier_state.value()));
        }
    }
}

mircv::XKBMapperRegistrar::XkbMappingState* mircv::XKBMapperRegistrar::get_keymapping_state(MirInputDeviceId id)
{
    auto dev_keymap = device_mapping.find(id);

    if (dev_keymap != end(device_mapping))
    {
        return dev_keymap->second.get();
    }
    if (default_keymap)
    {
        auto mapping_state = std::make_unique<XkbMappingState>(
            default_keymap, default_compiled_keymap, executor);
        decltype(device_mapping.begin()) insertion_pos;
        std::tie(insertion_pos, std::ignore) =
            device_mapping.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(std::move(mapping_state)));

        return insertion_pos->second.get();
    }
    return nullptr;
}

void mircv::XKBMapperRegistrar::set_keymap_for_all_devices(std::shared_ptr<Keymap> new_keymap)
{
    set_keymap(new_keymap);
}

void mircv::XKBMapperRegistrar::set_keymap(std::shared_ptr<Keymap> new_keymap)
{
    std::lock_guard lg(guard);
    default_keymap = std::move(new_keymap);
    default_compiled_keymap = default_keymap->make_unique_xkb_keymap(context.get());
    device_mapping.clear();
}

void mircv::XKBMapperRegistrar::set_keymap_for_device(MirInputDeviceId id, std::shared_ptr<Keymap> new_keymap)
{
    set_keymap(id, std::move(new_keymap));
}

void mircv::XKBMapperRegistrar::set_keymap(MirInputDeviceId id, std::shared_ptr<Keymap> new_keymap)
{
    std::lock_guard lg(guard);

    auto compiled_keymap = new_keymap->make_unique_xkb_keymap(context.get());
    auto mapping_state = std::make_unique<XkbMappingState>(std::move(new_keymap), std::move(compiled_keymap), executor);

    device_mapping.erase(id);
    device_mapping.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(std::move(mapping_state)));
}

void mircv::XKBMapperRegistrar::clear_all_keymaps()
{
    std::lock_guard lg(guard);
    default_keymap.reset();
    device_mapping.clear();
    update_modifier();
}

void mircv::XKBMapperRegistrar::clear_keymap_for_device(MirInputDeviceId id)
{
    std::lock_guard lg(guard);
    device_mapping.erase(id);
    update_modifier();
}

MirInputEventModifiers mircv::XKBMapperRegistrar::modifiers() const
{
    std::lock_guard lg(guard);
    if (modifier_state.is_set())
        return expand_modifiers(modifier_state.value());
    return mir_input_event_modifier_none;
}

MirInputEventModifiers mircv::XKBMapperRegistrar::device_modifiers(MirInputDeviceId id) const
{
    std::lock_guard lg(guard);

    auto it = device_mapping.find(id);
    if (it == end(device_mapping))
        return mir_input_event_modifier_none;
    return expand_modifiers(it->second->modifiers());
}

auto mircv::XKBMapperRegistrar::xkb_modifiers() const -> MirXkbModifiers
{
    std::lock_guard lg(guard);
    return xkb_modifiers_;
}

void mircv::XKBMapperRegistrar::register_interest(std::weak_ptr<LedObserver> const& observer, MirInputDeviceId id)
{
    auto it = device_mapping.find(id);
    if (it == end(device_mapping))
        return;

    auto& registrar = it->second->get_registrar();
    registrar.register_interest(observer);
}

void mircv::XKBMapperRegistrar::unregister_interest(LedObserver const& observer, MirInputDeviceId id)
{
    auto it = device_mapping.find(id);
    if (it == end(device_mapping))
        return;

    auto& registrar = it->second->get_registrar();
    registrar.unregister_interest(observer);
}

mircv::XKBMapperRegistrar::XkbMappingState::XkbMappingState(
    std::shared_ptr<Keymap> keymap,
    std::shared_ptr<xkb_keymap> compiled_keymap,
    Executor& executor)
    : keymap{std::move(keymap)},
      compiled_keymap{std::move(compiled_keymap)},
      state{make_unique_state(this->compiled_keymap.get())},
      num_led{xkb_keymap_led_get_index(this->compiled_keymap.get(), XKB_LED_NAME_NUM)},
      caps_led{xkb_keymap_led_get_index(this->compiled_keymap.get(), XKB_LED_NAME_CAPS)},
      scroll_led{xkb_keymap_led_get_index(this->compiled_keymap.get(), XKB_LED_NAME_SCROLL)},
      registrar(executor)
{
}

void mircv::XKBMapperRegistrar::XkbMappingState::set_key_state(std::vector<uint32_t> const& key_state)
{
    modifier_state = mir_input_event_modifier_none;
    std::unordered_set<uint32_t> pressed_codes;
    std::string t;
    for (uint32_t scan_code : key_state)
    {
        bool already_pressed = pressed_codes.count(scan_code) > 0;

        update_state(to_xkb_scan_code(scan_code),
                     (already_pressed) ? mir_keyboard_action_up : mir_keyboard_action_down,
                     nullptr,
                     t);

        if (already_pressed)
            pressed_codes.erase(scan_code);
        else
            pressed_codes.insert(scan_code);
    }
}

bool mircv::XKBMapperRegistrar::XkbMappingState::update_and_map(MirEvent& event, mircv::XKBMapperRegistrar::ComposeState* compose_state)
{
    auto& key_ev = *event.to_input()->to_keyboard();
    uint32_t xkb_scan_code = to_xkb_scan_code(key_ev.scan_code());
    auto old_state = modifier_state;
    std::string key_text;
    auto const update_result = update_state(xkb_scan_code, key_ev.action(), compose_state, key_text);
    xkb_keysym_t const keysym = update_result.first;
    bool const xkb_modifiers_updated = update_result.second;

    key_ev.set_keysym(keysym);
    key_ev.set_text(key_text.c_str());
    // TODO we should also indicate effective/consumed modifier state to properly
    // implement short cuts with keys that are only reachable via modifier keys
    key_ev.set_modifiers(expand_modifiers(modifier_state));
    key_ev.set_keymap(keymap);

    return old_state != modifier_state || xkb_modifiers_updated;
}

MirInputEventModifiers mircv::XKBMapperRegistrar::XkbMappingState::modifiers() const
{
    return modifier_state;
}

auto mircv::XKBMapperRegistrar::XkbMappingState::xkb_modifiers() const -> MirXkbModifiers
{
    return MirXkbModifiers{
        xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_DEPRESSED),
        xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_LATCHED),
        xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_LOCKED),
        xkb_state_serialize_layout(state.get(), XKB_STATE_LAYOUT_EFFECTIVE)};
}

mi::KeyboardLeds mircv::XKBMapperRegistrar::XkbMappingState::get_leds() const
{
    KeyboardLeds leds;
    if (xkb_state_led_index_is_active(state.get(), caps_led))
        leds |= KeyboardLed::caps_lock;
    if (xkb_state_led_index_is_active(state.get(), num_led))
        leds |= KeyboardLed::num_lock;
    if (xkb_state_led_index_is_active(state.get(), scroll_led))
        leds |= KeyboardLed::scroll_lock;
    return leds;
}

mircv::XKBMapperRegistrar::XkbMappingState::XKBMappingStateRegistrar& mircv::XKBMapperRegistrar::XkbMappingState::get_registrar()
{
    return registrar;
}

auto mircv::XKBMapperRegistrar::XkbMappingState::update_state(
    uint32_t scan_code,
    MirKeyboardAction action,
    mircv::XKBMapperRegistrar::ComposeState* compose_state,
    std::string& text)
-> std::pair<xkb_keysym_t, bool>
{
    auto keysym = xkb_state_key_get_one_sym(state.get(), scan_code);
    auto const mod_change = modifier_from_xkb_scan_code(scan_code);

    // Occasionally, we see XKB_KEY_Meta_L where XKB_KEY_Alt_L is correct
    if (mod_change == mir_input_event_modifier_alt_left)
    {
        keysym = XKB_KEY_Alt_L;
    }

    if(action == mir_keyboard_action_down || action == mir_keyboard_action_repeat)
    {
        char buffer[7];
        // scan code? really? not keysym?
        xkb_state_key_get_utf8(state.get(), scan_code, buffer, sizeof(buffer));
        text = buffer;
    }

    if (compose_state)
        keysym = compose_state->update_state(keysym, action, text);

    uint32_t mask{0};
    if (action == mir_keyboard_action_up)
    {
        mask = xkb_state_update_key(state.get(), scan_code, XKB_KEY_UP);
        // TODO get the modifier state from xkbcommon and apply it
        // for all other modifiers manually track them here:
        release_modifier(mod_change);
    }
    else if (action == mir_keyboard_action_down)
    {
        mask = xkb_state_update_key(state.get(), scan_code, XKB_KEY_DOWN);
        // TODO get the modifier state from xkbcommon and apply it
        // for all other modifiers manually track them here:
        press_modifier(mod_change);
    }

    bool const xkb_modifiers_changed =
        (mask & XKB_STATE_MODS_DEPRESSED) ||
        (mask & XKB_STATE_MODS_LATCHED) ||
        (mask & XKB_STATE_MODS_LOCKED) ||
        (mask & XKB_STATE_LAYOUT_EFFECTIVE);

    return {keysym, xkb_modifiers_changed};
}

mircv::XKBMapperRegistrar::ComposeState* mircv::XKBMapperRegistrar::get_compose_state(MirInputDeviceId id)
{
    auto dev_compose_state = device_composing.find(id);

    if (dev_compose_state != end(device_composing))
    {
        return dev_compose_state->second.get();
    }
    if (compose_table)
    {
        decltype(device_composing.begin()) insertion_pos;
        std::tie(insertion_pos, std::ignore) =
            device_composing.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(id),
                                     std::forward_as_tuple(std::make_unique<ComposeState>(compose_table)));

        return insertion_pos->second.get();
    }
    return nullptr;
}

mircv::XKBMapperRegistrar::ComposeState::ComposeState(XKBComposeTablePtr const& table) :
    state{make_unique_compose_state(table)}
{
}

xkb_keysym_t mircv::XKBMapperRegistrar::ComposeState::update_state(xkb_keysym_t mapped_key, MirKeyboardAction action, std::string& mapped_text)
{
    // the state machine only cares about downs
    if (action == mir_keyboard_action_down)
    {
        if (xkb_compose_state_feed(state.get(), mapped_key) == XKB_COMPOSE_FEED_ACCEPTED)
        {
            auto result = xkb_compose_state_get_status(state.get());
            if (result == XKB_COMPOSE_COMPOSED)
            {
                char buffer[7];
                xkb_compose_state_get_utf8(state.get(), buffer, sizeof(buffer));
                mapped_text = buffer;
                auto composed_key_sym = xkb_compose_state_get_one_sym(state.get());
                last_composed_key = std::make_tuple(mapped_key, composed_key_sym);
                return composed_key_sym;
            }
            else if (result == XKB_COMPOSE_COMPOSING)
            {
                mapped_text = "";
                consumed_keys.insert(mapped_key);
                return XKB_KEY_NoSymbol;
            }
            else if (result == XKB_COMPOSE_CANCELLED)
            {
                consumed_keys.insert(mapped_key);
                return XKB_KEY_NoSymbol;
            }
        }
    }
    else
    {
        if (last_composed_key.is_set() &&
            mapped_key == std::get<0>(last_composed_key.value()))
        {
            if (action == mir_keyboard_action_up)
            {
                mapped_text = "";
                return std::get<1>(last_composed_key.consume());
            }
            else
            {
                // on repeat get the text of the compose state
                char buffer[7];
                xkb_compose_state_get_utf8(state.get(), buffer, sizeof(buffer));
                mapped_text = buffer;
                return std::get<1>(last_composed_key.value());
            }
        }
        if (consumed_keys.erase(mapped_key))
        {
            mapped_text = "";
            return XKB_KEY_NoSymbol;
        }
    }

    return mapped_key;
}


void mircv::XKBMapperRegistrar::XkbMappingState::press_modifier(MirInputEventModifiers mod)
{
    if (is_toggle_modifier(mod))
        modifier_state ^= mod;
    else
        modifier_state |= mod;
}

void mircv::XKBMapperRegistrar::XkbMappingState::release_modifier(MirInputEventModifiers mod)
{
    if (!is_toggle_modifier(mod))
        modifier_state &= ~mod;
}
