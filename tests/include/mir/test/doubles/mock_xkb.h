/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_TEST_DOUBLES_MOCK_XKB_H_
#define MIR_TEST_DOUBLES_MOCK_XKB_H_

#include <gmock/gmock.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

struct xcb_connection_t;

namespace mir
{
namespace test
{
namespace doubles
{

class MockXkb
{
public:
    MockXkb();
    ~MockXkb();

    MOCK_METHOD(int32_t, xkb_x11_get_core_keyboard_device_id, (xcb_connection_t*), ());
    MOCK_METHOD(xkb_keymap*, xkb_x11_keymap_new_from_device, (xkb_context*,
        xcb_connection_t*,
        int32_t,
        xkb_keymap_compile_flags), ());
    MOCK_METHOD(xkb_state*, xkb_x11_state_new_from_device, (xkb_keymap*, xcb_connection_t*, int32_t), ());
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_XKB_H_
