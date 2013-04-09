/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "xkb_mapper.h"

#include <string.h>

namespace mcli = mir::client::input;

mcli::XKBMapper::XKBMapper()
{
    xkb_rule_names names;
    names.rules = "evdev";
    names.model = "pc105";
    names.layout = "us";
    names.variant = "";
    names.options = "";

    context = xkb_context_new(xkb_context_flags(0));
    map = xkb_map_new_from_names(context, &names,
                                     xkb_map_compile_flags(0));
    state = xkb_state_new(map);
}

mcli::XKBMapper::~XKBMapper()
{
    xkb_state_unref(state);
    xkb_map_unref(map);
    xkb_context_unref(context);
}

namespace
{

// xkb scancodes are offset by 8 from evdev scancodes for compatibility with X protocol.
static uint32_t const xkb_scancode_offset_from_evdev = 8;

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
    uint32_t xkb_scan_code = scan_code + xkb_scancode_offset_from_evdev;
    xkb_state_update_key(state, xkb_scan_code, XKB_KEY_DOWN);
    
    return keysym_for_scan_code(state, xkb_scan_code);
}

xkb_keysym_t mcli::XKBMapper::release_and_map_key(int scan_code)
{
    uint32_t xkb_scan_code = scan_code + xkb_scancode_offset_from_evdev;
    xkb_state_update_key(state, xkb_scan_code, XKB_KEY_UP);
    
    return keysym_for_scan_code(state, xkb_scan_code);
}
