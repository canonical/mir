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

namespace mcli = mir::client::input;


mcli::XKBMapper::XKBMapper()
{
}

mcli::XKBMapper::~XKBMapper()
{
}

xkb_keysym_t mcli::XKBMapper::press_and_map_key(int scan_code)
{
    // TODO
    return (xkb_keysym_t) scan_code;
}

xkb_keysym_t mcli::XKBMapper::release_and_map_key(int scan_code)
{
    // TODO
    return (xkb_keysym_t) scan_code;
}
