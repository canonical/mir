/*
 * Copyright © Canonical Ltd.
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
 */

#include <mir/input/xkb_mapper.h>
#include <mir/input/keymap.h>
#include <mir/events/event_private.h>
#include <mir/events/event_builders.h>

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

mircv::XKBMapper::XKBMapper() :
    context{make_unique_context()},
    compose_table{make_unique_compose_table_from_locale(context, get_locale_from_environment())}
{
}

void mircv::XKBMapper::set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& key_state)
{
    std::lock_guard lg(guard);

    auto mapping_state = get_keymapping_state(id);
    if (mapping_state)
        mapping_state->set_key_state(key_state);
}

void mircv::XKBMapper::update_modifier()
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

void mircv::XKBMapper::map_event(MirEvent& ev)
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

mircv::XKBMapper::XkbMappingState* mircv::XKBMapper::get_keymapping_state(MirInputDeviceId id)
{
    auto dev_keymap = device_mapping.find(id);

    if (dev_keymap != end(device_mapping))
    {
        return dev_keymap->second.get();
    }
    if (default_keymap)
    {
        auto mapping_state = std::make_unique<XkbMappingState>(default_keymap, default_compiled_keymap);
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

void mircv::XKBMapper::set_keymap_for_all_devices(std::shared_ptr<Keymap> new_keymap)
{
    set_keymap(new_keymap);
}

void mircv::XKBMapper::set_keymap(std::shared_ptr<Keymap> new_keymap)
{
    std::lock_guard lg(guard);
    default_keymap = std::move(new_keymap);
    default_compiled_keymap = default_keymap->make_unique_xkb_keymap(context.get());
    device_mapping.clear();
}

void mircv::XKBMapper::set_keymap_for_device(MirInputDeviceId id, std::shared_ptr<Keymap> new_keymap)
{
    set_keymap(id, std::move(new_keymap));
}

void mircv::XKBMapper::set_keymap(MirInputDeviceId id, std::shared_ptr<Keymap> new_keymap)
{
    std::lock_guard lg(guard);

    auto compiled_keymap = new_keymap->make_unique_xkb_keymap(context.get());
    auto mapping_state = std::make_unique<XkbMappingState>(std::move(new_keymap), std::move(compiled_keymap));

    device_mapping.erase(id);
    device_mapping.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(std::move(mapping_state)));
}

void mircv::XKBMapper::clear_all_keymaps()
{
    std::lock_guard lg(guard);
    default_keymap.reset();
    device_mapping.clear();
    update_modifier();
}

void mircv::XKBMapper::clear_keymap_for_device(MirInputDeviceId id)
{
    std::lock_guard lg(guard);
    device_mapping.erase(id);
    update_modifier();
}

MirInputEventModifiers mircv::XKBMapper::modifiers() const
{
    std::lock_guard lg(guard);
    if (modifier_state.is_set())
        return expand_modifiers(modifier_state.value());
    return mir_input_event_modifier_none;
}

MirInputEventModifiers mircv::XKBMapper::device_modifiers(MirInputDeviceId id) const
{
    std::lock_guard lg(guard);

    auto it = device_mapping.find(id);
    if (it == end(device_mapping))
        return mir_input_event_modifier_none;
    return expand_modifiers(it->second->modifiers());
}

auto mircv::XKBMapper::xkb_modifiers() const -> MirXkbModifiers
{
    std::lock_guard lg(guard);
    return xkb_modifiers_;
}

mircv::XKBMapper::XkbMappingState::XkbMappingState(
    std::shared_ptr<Keymap> keymap,
    std::shared_ptr<xkb_keymap> compiled_keymap)
    : keymap{std::move(keymap)},
      compiled_keymap{std::move(compiled_keymap)},
      state{make_unique_state(this->compiled_keymap.get())}
{
    // Cache XKB modifier indices for efficient lookup
    mod_index_shift = xkb_keymap_mod_get_index(this->compiled_keymap.get(), XKB_MOD_NAME_SHIFT);
    mod_index_ctrl = xkb_keymap_mod_get_index(this->compiled_keymap.get(), XKB_MOD_NAME_CTRL);
    mod_index_alt = xkb_keymap_mod_get_index(this->compiled_keymap.get(), XKB_MOD_NAME_ALT);
    mod_index_logo = xkb_keymap_mod_get_index(this->compiled_keymap.get(), XKB_MOD_NAME_LOGO);
    mod_index_caps = xkb_keymap_mod_get_index(this->compiled_keymap.get(), XKB_MOD_NAME_CAPS);
    mod_index_num = xkb_keymap_mod_get_index(this->compiled_keymap.get(), XKB_MOD_NAME_NUM);
}

void mircv::XKBMapper::XkbMappingState::set_key_state(std::vector<uint32_t> const& key_state)
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

bool mircv::XKBMapper::XkbMappingState::update_and_map(MirEvent& event, mircv::XKBMapper::ComposeState* compose_state)
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

MirInputEventModifiers mircv::XKBMapper::XkbMappingState::modifiers() const
{
    return modifier_state;
}

auto mircv::XKBMapper::XkbMappingState::xkb_modifiers() const -> MirXkbModifiers
{
    return MirXkbModifiers{
        xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_DEPRESSED),
        xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_LATCHED),
        xkb_state_serialize_mods(state.get(), XKB_STATE_MODS_LOCKED),
        xkb_state_serialize_layout(state.get(), XKB_STATE_LAYOUT_EFFECTIVE)};
}

auto mircv::XKBMapper::XkbMappingState::update_state(
    uint32_t scan_code,
    MirKeyboardAction action,
    mircv::XKBMapper::ComposeState* compose_state,
    std::string& text)
-> std::pair<xkb_keysym_t, bool>
{
    auto keysym = xkb_state_key_get_one_sym(state.get(), scan_code);

    if(action == mir_keyboard_action_down || action == mir_keyboard_action_repeat)
    {
        char buffer[7];
        xkb_state_key_get_utf8(state.get(), scan_code, buffer, sizeof(buffer));
        text = buffer;
    }

    if (compose_state)
        keysym = compose_state->update_state(keysym, action, text);

    uint32_t mask{0};
    if (action == mir_keyboard_action_up)
    {
        mask = xkb_state_update_key(state.get(), scan_code, XKB_KEY_UP);
        pressed_scancodes.erase(scan_code);
        // Query XKB for the actual modifier state after key update
        update_modifier_from_xkb_state();
    }
    else if (action == mir_keyboard_action_down)
    {
        mask = xkb_state_update_key(state.get(), scan_code, XKB_KEY_DOWN);
        pressed_scancodes.insert(scan_code);
        // Query XKB for the actual modifier state after key update
        update_modifier_from_xkb_state();
    }

    bool const xkb_modifiers_changed =
        (mask & XKB_STATE_MODS_DEPRESSED) ||
        (mask & XKB_STATE_MODS_LATCHED) ||
        (mask & XKB_STATE_MODS_LOCKED) ||
        (mask & XKB_STATE_LAYOUT_EFFECTIVE);

    return {keysym, xkb_modifiers_changed};
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

void mircv::XKBMapper::XkbMappingState::update_modifier_from_xkb_state()
{
    // Query XKB for the actual modifier state instead of tracking manually by scan codes
    // This ensures that XKB options like altwin:swap_alt_win are properly respected
    MirInputEventModifiers new_modifier_state = mir_input_event_modifier_none;
    
    // Check shift modifier and determine left/right based on which physical keys are pressed
    if (mod_index_shift != XKB_MOD_INVALID && 
        xkb_state_mod_index_is_active(state.get(), mod_index_shift, XKB_STATE_MODS_EFFECTIVE))
    {
        new_modifier_state |= mir_input_event_modifier_shift;
        // Check which physical shift keys are pressed
        if (pressed_scancodes.find(to_xkb_scan_code(KEY_LEFTSHIFT)) != pressed_scancodes.end())
            new_modifier_state |= mir_input_event_modifier_shift_left;
        if (pressed_scancodes.find(to_xkb_scan_code(KEY_RIGHTSHIFT)) != pressed_scancodes.end())
            new_modifier_state |= mir_input_event_modifier_shift_right;
    }
    
    // Check ctrl modifier
    if (mod_index_ctrl != XKB_MOD_INVALID &&
        xkb_state_mod_index_is_active(state.get(), mod_index_ctrl, XKB_STATE_MODS_EFFECTIVE))
    {
        new_modifier_state |= mir_input_event_modifier_ctrl;
        // Check which physical ctrl keys are pressed
        if (pressed_scancodes.find(to_xkb_scan_code(KEY_LEFTCTRL)) != pressed_scancodes.end())
            new_modifier_state |= mir_input_event_modifier_ctrl_left;
        if (pressed_scancodes.find(to_xkb_scan_code(KEY_RIGHTCTRL)) != pressed_scancodes.end())
            new_modifier_state |= mir_input_event_modifier_ctrl_right;
    }
    
    // Check alt and meta modifiers - these are where altwin:swap_alt_win affects behavior
    // XKB tells us if Alt/Meta are active, we determine left/right based on keysyms of pressed keys
    bool const alt_active = mod_index_alt != XKB_MOD_INVALID &&
        xkb_state_mod_index_is_active(state.get(), mod_index_alt, XKB_STATE_MODS_EFFECTIVE);
    bool const meta_active = mod_index_logo != XKB_MOD_INVALID &&
        xkb_state_mod_index_is_active(state.get(), mod_index_logo, XKB_STATE_MODS_EFFECTIVE);
    
    if (alt_active || meta_active)
    {
        if (alt_active)
            new_modifier_state |= mir_input_event_modifier_alt;
        if (meta_active)
            new_modifier_state |= mir_input_event_modifier_meta;
        
        // Single iteration to check all pressed keys for both Alt and Meta modifiers.
        // We query the keysym of each pressed key because XKB options like altwin:swap_alt_win
        // change which keysyms are produced (e.g., physical Win key produces Alt_L keysym).
        // This allows us to correctly determine left/right variants after key remapping.
        for (auto scan_code : pressed_scancodes)
        {
            auto keysym = xkb_state_key_get_one_sym(state.get(), scan_code);
            
            if (alt_active)
            {
                if (keysym == XKB_KEY_Alt_L)
                    new_modifier_state |= mir_input_event_modifier_alt_left;
                else if (keysym == XKB_KEY_Alt_R)
                    new_modifier_state |= mir_input_event_modifier_alt_right;
            }
            
            if (meta_active)
            {
                if (keysym == XKB_KEY_Super_L || keysym == XKB_KEY_Meta_L)
                    new_modifier_state |= mir_input_event_modifier_meta_left;
                else if (keysym == XKB_KEY_Super_R || keysym == XKB_KEY_Meta_R)
                    new_modifier_state |= mir_input_event_modifier_meta_right;
            }
        }
    }
    
    // Check caps lock
    if (mod_index_caps != XKB_MOD_INVALID &&
        xkb_state_mod_index_is_active(state.get(), mod_index_caps, XKB_STATE_MODS_EFFECTIVE))
    {
        new_modifier_state |= mir_input_event_modifier_caps_lock;
    }
    
    // Check num lock
    if (mod_index_num != XKB_MOD_INVALID &&
        xkb_state_mod_index_is_active(state.get(), mod_index_num, XKB_STATE_MODS_EFFECTIVE))
    {
        new_modifier_state |= mir_input_event_modifier_num_lock;
    }
    
    modifier_state = new_modifier_state;
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
