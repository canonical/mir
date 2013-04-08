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

#ifndef MIR_CLIENT_INPUT_XKB_MAPPER_H_
#define MIR_CLIENT_INPUT_XKB_MAPPER_H_

#include <xkbcommon/xkbcommon.h>

namespace mir
{
namespace client
{
namespace input
{

class XKBMapper
{
public:
    XKBMapper();
    virtual ~XKBMapper();
    
    xkb_keysym_t press_and_map_key(int scan_code);
    xkb_keysym_t release_and_map_key(int scan_code);

protected:
    XKBMapper(XKBMapper const&) = delete;
    XKBMapper& operator=(XKBMapper const&) = delete;

private:
    xkb_context *context;
    xkb_keymap *map;
    xkb_state *state;
};

}
}
}

#endif // MIR_CLIENT_INPUT_XKB_MAPPER_H_
