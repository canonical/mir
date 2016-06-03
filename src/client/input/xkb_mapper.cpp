/*
 * Copyright © 2013-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include <boost/throw_exception.hpp>
#include <iostream>

namespace mi = mir::input;
namespace mev = mir::events;
namespace mircv = mi::receiver;

namespace
{

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
    case XKB_KEY_Meta_L: return mir_input_event_modifier_meta_left;
    case XKB_KEY_Meta_R: return mir_input_event_modifier_meta_right;
    case XKB_KEY_Caps_Lock: return mir_input_event_modifier_caps_lock;
    case XKB_KEY_Scroll_Lock: return mir_input_event_modifier_scroll_lock;
    default: return MirInputEventModifiers{0};
    }
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

xkb_keysym_t keysym_for_scan_code(xkb_state* state, uint32_t xkb_scan_code)
{
    const xkb_keysym_t* syms;
    uint32_t num_syms = xkb_key_get_syms(state, xkb_scan_code, &syms);

    if (num_syms == 1)
    {
        return syms[0];
    }

    return XKB_KEY_NoSymbol;
}

}

mi::XKBContextPtr mi::make_unique_context()
{
    return {xkb_context_new(xkb_context_flags(0)), &xkb_context_unref};
}

mi::XKBKeymapPtr mi::make_unique_keymap(xkb_context* context, mi::Keymap const& map)
{
    xkb_rule_names keymap_names
    {
        "evdev",
        map.model.c_str(),
        map.layout.c_str(),
        map.variant.c_str(),
        map.options.c_str()
    };
    return {xkb_keymap_new_from_names(context, &keymap_names, xkb_keymap_compile_flags(0)), &xkb_keymap_unref};
}

mi::XKBKeymapPtr mi::make_unique_keymap(xkb_context* context, char const* buffer, size_t size)
{
    return {xkb_keymap_new_from_buffer(context, buffer, size, XKB_KEYMAP_FORMAT_TEXT_V1, xkb_keymap_compile_flags(0)),
            &xkb_keymap_unref};
}

using XKBStatePtr = std::unique_ptr<xkb_state, void(*)(xkb_state*)>;
mi::XKBStatePtr mi::make_unique_state(xkb_keymap* keymap)
{
    return {xkb_state_new(keymap), xkb_state_unref};
}

mircv::XKBMapper::XKBMapper()
    : context{make_unique_context()}
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
    if (surface_mapping)
    {
        modifier_state = surface_mapping->modifier_state;
    }
    else if (!device_mapping.empty())
    {
        MirInputEventModifiers new_modifier = 0;
        for (auto const& mapping_state : device_mapping)
        {
            new_modifier |= mapping_state.second.modifier_state;
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
        auto mapping_state = get_keymapping_state(device_id);

        if (input_type == mir_input_event_type_key && mapping_state)
        {
            if (mapping_state->update_and_map(ev))
                update_modifier();
        }
        else if (modifier_state.is_set())
        {
            mev::set_modifier(ev, expand_modifiers(modifier_state.value()));
        }
    }
}

mircv::XKBMapper::XkbMappingState* mircv::XKBMapper::get_keymapping_state(MirInputDeviceId id)
{
    if (surface_mapping)
    {
        return surface_mapping.get();
    }
    else
    {
        auto dev_keymap = device_mapping.find(id);

        if (dev_keymap != end(device_mapping))
        {
            return &dev_keymap->second;
        }
    }

    return nullptr;
}

void mircv::XKBMapper::set_keymap(Keymap const& new_keymap)
{
    set_keymap(make_unique_keymap(context.get(), new_keymap));
}

void mircv::XKBMapper::set_keymap(char const* buffer, size_t len)
{
    set_keymap(make_unique_keymap(context.get(), buffer, len));
}

void mircv::XKBMapper::set_keymap(XKBKeymapPtr new_keymap)
{
    std::lock_guard<std::mutex> lg(guard);
    surface_mapping = std::make_unique<XkbMappingState>(std::move(new_keymap));
}

void mircv::XKBMapper::set_keymap(MirInputDeviceId id, Keymap const& new_keymap)
{
    set_keymap(id, make_unique_keymap(context.get(), new_keymap));
}

void mircv::XKBMapper::set_keymap(MirInputDeviceId id, char const* buffer, size_t len)
{
    set_keymap(id, make_unique_keymap(context.get(), buffer, len));
}

void mircv::XKBMapper::set_keymap(MirInputDeviceId id, XKBKeymapPtr new_keymap)
{
    std::lock_guard<std::mutex> lg(guard);

    device_mapping.emplace(std::piecewise_construct,
                           std::forward_as_tuple(id),
                           std::forward_as_tuple(std::move(new_keymap)));
}

void mircv::XKBMapper::reset_keymap()
{
    std::lock_guard<std::mutex> lg(guard);
    surface_mapping.reset();
    update_modifier();
}

void mircv::XKBMapper::reset_keymap(MirInputDeviceId id)
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

mircv::XKBMapper::XkbMappingState::XkbMappingState(XKBKeymapPtr keymap)
    : keymap{std::move(keymap)}, state{make_unique_state(this->keymap.get())}
{
}

void mircv::XKBMapper::XkbMappingState::set_key_state(std::vector<uint32_t> const& key_state)
{
    state = make_unique_state(keymap.get());
    modifier_state = mir_input_event_modifier_none;
    for (uint32_t scan_code : key_state)
        update_state(to_xkb_scan_code(scan_code), mir_keyboard_action_down);
}

bool mircv::XKBMapper::XkbMappingState::update_and_map(MirEvent& event)
{
    auto& key_ev = *event.to_input()->to_keyboard();
    // TODO test if key entry is start of a compose key sequence..
    // then use compose key API to map key..
    uint32_t xkb_scan_code = to_xkb_scan_code(key_ev.scan_code());
    bool old_state = modifier_state;

    key_ev.set_key_code(update_state(xkb_scan_code, key_ev.action()));
    // TODO we should also indicate effective/consumed modifier state to properly
    // implement short cuts with keys that are only reachable via modifier keys
    key_ev.set_modifiers(expand_modifiers(modifier_state));

    return old_state != modifier_state;
}

xkb_keysym_t mircv::XKBMapper::XkbMappingState::update_state(uint32_t scan_code, MirKeyboardAction action)
{
    auto key_sym = keysym_for_scan_code(state.get(), scan_code);
    auto mod_change = xkb_key_code_to_modifier(key_sym);

    if (action == mir_keyboard_action_up)
    {
        xkb_state_update_key(state.get(), scan_code, XKB_KEY_UP);
        modifier_state = modifier_state & ~mod_change;
    }
    else if (action == mir_keyboard_action_down)
    {
        xkb_state_update_key(state.get(), scan_code, XKB_KEY_DOWN);
        modifier_state = modifier_state | mod_change;
    }

    return key_sym;
}
