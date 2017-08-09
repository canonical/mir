/*
 * Copyright © 2013-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Robert Carr <robert.carr@canonical.com>
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/xkb_mapper.h"
#include "mir/input/keymap.h"
#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"

#include <sstream>
#include <boost/throw_exception.hpp>
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

MirInputEventModifiers xkb_key_code_to_modifier(xkb_keysym_t key)
{
    switch(key)
    {
    case XKB_KEY_Shift_R: return mir_input_event_modifier_shift_right;
    case XKB_KEY_Shift_L: return mir_input_event_modifier_shift_left;
    case XKB_KEY_Alt_R: return mir_input_event_modifier_alt_right;
    case XKB_KEY_Alt_L: return mir_input_event_modifier_alt_left;
    case XKB_KEY_Control_R: return mir_input_event_modifier_ctrl_right;
    case XKB_KEY_Control_L: return mir_input_event_modifier_ctrl_left;
    case XKB_KEY_Super_L: return mir_input_event_modifier_meta_left;
    case XKB_KEY_Super_R: return mir_input_event_modifier_meta_right;
    case XKB_KEY_Caps_Lock: return mir_input_event_modifier_caps_lock;
    case XKB_KEY_Scroll_Lock: return mir_input_event_modifier_scroll_lock;
    case XKB_KEY_Num_Lock: return mir_input_event_modifier_num_lock;
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


uint32_t to_xkb_scan_code(uint32_t evdev_scan_code)
{
    // xkb scancodes are offset by 8 from evdev scancodes for compatibility with X protocol.
    return evdev_scan_code + 8;
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
    return {xkb_context_new(xkb_context_flags(0)), &xkb_context_unref};
}

mi::XKBKeymapPtr mi::make_unique_keymap(xkb_context* context, mi::Keymap const& map)
{
    std::stringstream error;
    error << "Illegal keymap configuration evdev-" << map;
    xkb_rule_names keymap_names
    {
        "evdev",
        map.model.c_str(),
        map.layout.c_str(),
        map.variant.c_str(),
        map.options.c_str()
    };
    auto keymap_ptr = xkb_keymap_new_from_names(context, &keymap_names, xkb_keymap_compile_flags(0));

    if (!keymap_ptr)
        BOOST_THROW_EXCEPTION(std::invalid_argument(error.str().c_str()));
    return {keymap_ptr, &xkb_keymap_unref};
}

mi::XKBKeymapPtr mi::make_unique_keymap(xkb_context* context, char const* buffer, size_t size)
{
    auto keymap_ptr = xkb_keymap_new_from_buffer(context, buffer, size, XKB_KEYMAP_FORMAT_TEXT_V1, xkb_keymap_compile_flags(0));
    if (!keymap_ptr)
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to create keymap from buffer."));

    return {keymap_ptr, &xkb_keymap_unref};
}

mircv::XKBMapper::XKBMapper() :
    context{make_unique_context()},
    compose_table{make_unique_compose_table_from_locale(context, get_locale_from_environment())}
{
}

void mircv::XKBMapper::set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& key_state)
{
    std::lock_guard<std::mutex> lg(guard);

    auto mapping_state = get_keymapping_state(id);
    if (mapping_state)
        mapping_state->set_key_state(key_state);
}

void mircv::XKBMapper::update_modifier()
{
    modifier_state = mir::optional_value<MirInputEventModifiers>{};
    if (!device_mapping.empty())
    {
        MirInputEventModifiers new_modifier = 0;
        for (auto const& mapping_state : device_mapping)
        {
            new_modifier |= mapping_state.second->modifiers();
        }

        modifier_state = new_modifier;
    }
}

void mircv::XKBMapper::map_event(MirEvent& ev)
{
    std::lock_guard<std::mutex> lg(guard);

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
                auto compose_state = get_compose_state(device_id);
                if (mapping_state->update_and_map(ev, compose_state))
                    update_modifier();
            }
        }
        else if (modifier_state.is_set())
        {
            mev::set_modifier(ev, expand_modifiers(modifier_state.value()));
        }
    }
}

mircv::XKBMapper::XkbMappingState* mircv::XKBMapper::get_keymapping_state(MirInputDeviceId id)
{
    auto dev_keymap = device_mapping.find(id);

    if (dev_keymap != end(device_mapping))
    {
        return dev_keymap->second.get();
    }
    if (default_keymap)
    {
        decltype(device_mapping.begin()) insertion_pos;
        std::tie(insertion_pos, std::ignore) =
            device_mapping.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(id),
                                   std::forward_as_tuple(std::make_unique<XkbMappingState>(default_keymap)));

        return insertion_pos->second.get();
    }
    return nullptr;
}

void mircv::XKBMapper::set_keymap_for_all_devices(Keymap const& new_keymap)
{
    set_keymap(make_unique_keymap(context.get(), new_keymap));
}

void mircv::XKBMapper::set_keymap_for_all_devices(char const* buffer, size_t len)
{
    set_keymap(make_unique_keymap(context.get(), buffer, len));
}

void mircv::XKBMapper::set_keymap(XKBKeymapPtr new_keymap)
{
    std::lock_guard<std::mutex> lg(guard);
    default_keymap = std::move(new_keymap);
    device_mapping.clear();
}

void mircv::XKBMapper::set_keymap_for_device(MirInputDeviceId id, Keymap const& new_keymap)
{
    set_keymap(id, make_unique_keymap(context.get(), new_keymap));
}

void mircv::XKBMapper::set_keymap_for_device(MirInputDeviceId id, char const* buffer, size_t len)
{
    set_keymap(id, make_unique_keymap(context.get(), buffer, len));
}

void mircv::XKBMapper::set_keymap(MirInputDeviceId id, XKBKeymapPtr new_keymap)
{
    std::lock_guard<std::mutex> lg(guard);

    device_mapping.erase(id);
    device_mapping.emplace(std::piecewise_construct,
                           std::forward_as_tuple(id),
                           std::forward_as_tuple(std::make_unique<XkbMappingState>(std::move(new_keymap))));
}

void mircv::XKBMapper::clear_all_keymaps()
{
    std::lock_guard<std::mutex> lg(guard);
    default_keymap.reset();
    device_mapping.clear();
    update_modifier();
}

void mircv::XKBMapper::clear_keymap_for_device(MirInputDeviceId id)
{
    std::lock_guard<std::mutex> lg(guard);
    device_mapping.erase(id);
    update_modifier();
}

MirInputEventModifiers mircv::XKBMapper::modifiers() const
{
    std::lock_guard<std::mutex> lg(guard);
    if (modifier_state.is_set())
        return expand_modifiers(modifier_state.value());
    return mir_input_event_modifier_none;
}

MirInputEventModifiers mircv::XKBMapper::device_modifiers(MirInputDeviceId id) const
{
    std::lock_guard<std::mutex> lg(guard);

    auto it = device_mapping.find(id);
    if (it == end(device_mapping))
        return mir_input_event_modifier_none;
    return expand_modifiers(it->second->modifiers());
}

mircv::XKBMapper::XkbMappingState::XkbMappingState(std::shared_ptr<xkb_keymap> const& keymap)
    : keymap{keymap}, state{make_unique_state(this->keymap.get())}
{
}

void mircv::XKBMapper::XkbMappingState::set_key_state(std::vector<uint32_t> const& key_state)
{
    state = make_unique_state(keymap.get());
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

bool mircv::XKBMapper::XkbMappingState::update_and_map(MirEvent& event, mircv::XKBMapper::ComposeState* compose_state)
{
    auto& key_ev = *event.to_input()->to_keyboard();
    uint32_t xkb_scan_code = to_xkb_scan_code(key_ev.scan_code());
    auto old_state = modifier_state;
    std::string key_text;
    xkb_keysym_t key_sym;
    key_sym = update_state(xkb_scan_code, key_ev.action(), compose_state, key_text);

    key_ev.set_key_code(key_sym);
    key_ev.set_text(key_text.c_str());
    // TODO we should also indicate effective/consumed modifier state to properly
    // implement short cuts with keys that are only reachable via modifier keys
    key_ev.set_modifiers(expand_modifiers(modifier_state));

    return old_state != modifier_state;
}

xkb_keysym_t mircv::XKBMapper::XkbMappingState::update_state(uint32_t scan_code, MirKeyboardAction action, mircv::XKBMapper::ComposeState* compose_state, std::string& text)
{
    auto key_sym = xkb_state_key_get_one_sym(state.get(), scan_code);
    auto mod_change = xkb_key_code_to_modifier(key_sym);

    if(action == mir_keyboard_action_down || action == mir_keyboard_action_repeat)
    {
        char buffer[7];
        // scan code? really? not keysym?
        xkb_state_key_get_utf8(state.get(), scan_code, buffer, sizeof(buffer));
        text = buffer;
    }

    if (compose_state)
        key_sym = compose_state->update_state(key_sym, action, text);

    if (action == mir_keyboard_action_up)
    {
        xkb_state_update_key(state.get(), scan_code, XKB_KEY_UP);
        // TODO get the modifier state from xkbcommon and apply it
        // for all other modifiers manually track them here:
        release_modifier(mod_change);
    }
    else if (action == mir_keyboard_action_down)
    {
        xkb_state_update_key(state.get(), scan_code, XKB_KEY_DOWN);
        // TODO get the modifier state from xkbcommon and apply it
        // for all other modifiers manually track them here:
        press_modifier(mod_change);
    }

    return key_sym;
}

MirInputEventModifiers mircv::XKBMapper::XkbMappingState::modifiers() const
{
    return modifier_state;
}

mircv::XKBMapper::ComposeState* mircv::XKBMapper::get_compose_state(MirInputDeviceId id)
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

mircv::XKBMapper::ComposeState::ComposeState(XKBComposeTablePtr const& table) :
    state{make_unique_compose_state(table)}
{
}

void mircv::XKBMapper::ComposeState::update_and_map(MirEvent& event)
{
    auto& key_ev = *event.to_input()->to_keyboard();

    auto const key_sym = key_ev.key_code();
    auto const action = key_ev.action();
    std::string text;
    key_ev.set_key_code(update_state(key_sym, action, text));
    key_ev.set_text(text.c_str());
}

xkb_keysym_t mircv::XKBMapper::ComposeState::update_state(xkb_keysym_t mapped_key, MirKeyboardAction action, std::string& mapped_text)
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


void mircv::XKBMapper::XkbMappingState::press_modifier(MirInputEventModifiers mod)
{
    if (is_toggle_modifier(mod))
        modifier_state ^= mod;
    else
        modifier_state |= mod;
}

void mircv::XKBMapper::XkbMappingState::release_modifier(MirInputEventModifiers mod)
{
    if (!is_toggle_modifier(mod))
        modifier_state &= ~mod;
}
