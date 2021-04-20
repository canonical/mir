/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "mir/test/doubles/mock_xkb.h"
#include <gtest/gtest.h>

#include <cstring>

namespace mtd=mir::test::doubles;

namespace
{
mtd::MockXkb* global_mock = nullptr;
}

mtd::MockXkb::MockXkb()
{
    using namespace testing;
    assert(global_mock == nullptr && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, xkb_x11_get_core_keyboard_device_id(_))
    .WillByDefault(Return(12));

    /*
    ON_CALL(*this, XGetGeometry(fake_x11.display,_,_,_,_,_,_,_,_))
    .WillByDefault(DoAll(SetArgPointee<5>(fake_x11.screen.width),
                         SetArgPointee<6>(fake_x11.screen.height),
                         Return(1)));
    */
}

mtd::MockXkb::~MockXkb()
{
    global_mock = nullptr;
}

int32_t xkb_x11_get_core_keyboard_device_id(xcb_connection_t* connection)
{
    return global_mock->xkb_x11_get_core_keyboard_device_id(connection);
}

xkb_keymap* xkb_x11_keymap_new_from_device(
    xkb_context* context,
    xcb_connection_t* connection,
    int32_t device_id,
    xkb_keymap_compile_flags flags)
{
    return global_mock->xkb_x11_keymap_new_from_device(context, connection, device_id, flags);
}

xkb_state* xkb_x11_state_new_from_device(
    xkb_keymap* keymap,
    xcb_connection_t* connection,
    int32_t device_id)
{
    return global_mock->xkb_x11_state_new_from_device(keymap, connection, device_id);
}
