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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/input/xkb_mapper.h"
#include "mir/events/event_private.h"

#include <boost/throw_exception.hpp>

namespace mi = mir::input;
namespace mircv = mi::receiver;

mi::XKBContextPtr mi::make_unique_context()
{
    return {xkb_context_new(xkb_context_flags(0)), &xkb_context_unref};
}

mi::XKBKeymapPtr mi::make_unique_keymap(xkb_context* context, std::string const& model, std::string const& layout,
                                std::string const& variant, std::string const& options)
{
    xkb_rule_names keymap_names
    {
        "evdev",
        model.c_str(),
        layout.c_str(),
        variant.c_str(),
        options.c_str()
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

namespace
{
void do_nothing_with_xkb_state(xkb_state*) {}
mi::XKBStatePtr make_empty_state()
{
    return {nullptr, &do_nothing_with_xkb_state};
}
}

mircv::XKBMapper::XKBMapper() :
    context(make_unique_context()),
    keymap(make_unique_keymap(context.get(), "pc105+inet", "us", "", "")),
    state(make_empty_state())
{
    if (keymap.get())
        state = make_unique_state(keymap.get());
    else
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create keymap"));
}

namespace
{

static uint32_t to_xkb_scan_code(uint32_t evdev_scan_code)
{
    // xkb scancodes are offset by 8 from evdev scancodes for compatibility with X protocol.
    return evdev_scan_code + 8;
}

static xkb_keysym_t keysym_for_scan_code(xkb_state* state, uint32_t xkb_scan_code)
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

void mircv::XKBMapper::update_state_and_map_event(MirEvent& ev)
{
    std::lock_guard<std::mutex> lg(guard);

    auto& key_ev = *ev.to_input()->to_keyboard();

    xkb_key_direction direction = XKB_KEY_DOWN;

    bool update_state = true;
    if (key_ev.action == mir_keyboard_action_up)
        direction = XKB_KEY_UP;
    else if (key_ev.action == mir_keyboard_action_down)
        direction = XKB_KEY_DOWN;
    else if (key_ev.action == mir_keyboard_action_repeat)
        update_state = false;

    uint32_t xkb_scan_code = to_xkb_scan_code(key_ev.scan_code);
    if (update_state)
        xkb_state_update_key(state.get(), xkb_scan_code, direction);

    key_ev.key_code = keysym_for_scan_code(state.get(), xkb_scan_code);
}

// id should be used in the future to implement per device keymaps
void mircv::XKBMapper::set_keymap(MirInputDeviceId /*id*/, XKBKeymapPtr new_keymap)
{
    std::lock_guard<std::mutex> lg(guard);

    if(new_keymap.get())
    {
        keymap = std::move(new_keymap);
        state = make_unique_state(keymap.get());
    }
}

xkb_context* mircv::XKBMapper::get_context() const
{
    return context.get();
}
