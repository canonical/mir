/*
 * Copyright © 2013 Canonical Ltd.
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

#include <string.h>

namespace mcli = mir::client::input;

namespace
{
struct XKBContextDeleter
{
    void operator()(xkb_context *c)
    {
        xkb_context_unref(c);
    }
};
struct XKBKeymapDeleter
{
    void operator()(xkb_keymap *k)
    {
        xkb_map_unref(k);
    }
};
struct XKBStateDeleter
{
    void operator()(xkb_state *s)
    {
        xkb_state_unref(s);
    }
};
}

mcli::XKBMapper::XKBMapper()
{
    xkb_rule_names names;
    names.rules = "evdev";
    names.model = "pc105";
    names.layout = "us";
    names.variant = "";
    names.options = "";

    context = std::shared_ptr<xkb_context>(xkb_context_new(xkb_context_flags(0)), XKBContextDeleter());
    map = std::shared_ptr<xkb_keymap>(xkb_map_new_from_names(context.get(), &names, xkb_map_compile_flags(0)), XKBKeymapDeleter());
    state = std::shared_ptr<xkb_state>(xkb_state_new(map.get()), XKBStateDeleter());
}

namespace
{

static uint32_t to_xkb_scan_code(uint32_t evdev_scan_code)
{
    // xkb scancodes are offset by 8 from evdev scancodes for compatibility with X protocol.
    return evdev_scan_code + 8;
}

static xkb_keysym_t keysym_for_scan_code(xkb_state *state, uint32_t xkb_scan_code)
{
    const xkb_keysym_t *syms;
    uint32_t num_syms = xkb_key_get_syms(state, xkb_scan_code, &syms);
    
    if (num_syms == 1)
    {
        return syms[0];
    }

    return XKB_KEY_NoSymbol;    
}

}

xkb_keysym_t mcli::XKBMapper::press_and_map_key(int scan_code)
{
    uint32_t xkb_scan_code = to_xkb_scan_code(scan_code);
    xkb_state_update_key(state.get(), xkb_scan_code, XKB_KEY_DOWN);
    
    return keysym_for_scan_code(state.get(), xkb_scan_code);
}

xkb_keysym_t mcli::XKBMapper::release_and_map_key(int scan_code)
{
    uint32_t xkb_scan_code = to_xkb_scan_code(scan_code);
    xkb_state_update_key(state.get(), xkb_scan_code, XKB_KEY_UP);
    
    return keysym_for_scan_code(state.get(), xkb_scan_code);
}
