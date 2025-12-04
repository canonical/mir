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

#include <mir/test/doubles/mock_xkb.h>
#include <gtest/gtest.h>

// xcb/xkb.h has a struct member named "explicit", which C++ does not like
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define explicit explicit_
#include <xcb/xkb.h>
#undef explicit
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace mtd=mir::test::doubles;

namespace
{
mtd::MockXkb* global_mock = nullptr;
}

struct xcb_extension_t {};
xcb_extension_t xcb_xkb_id;

xcb_xkb_use_extension_cookie_t
xcb_xkb_use_extension(xcb_connection_t*, uint16_t, uint16_t)
{
    return {0};
}

xcb_xkb_use_extension_reply_t *
xcb_xkb_use_extension_reply(xcb_connection_t*, xcb_xkb_use_extension_cookie_t, xcb_generic_error_t**)
{
    return nullptr;
}

xcb_void_cookie_t
xcb_xkb_select_events(
    xcb_connection_t*,
    xcb_xkb_device_spec_t,
    uint16_t,
    uint16_t,
    uint16_t,
    uint16_t,
    uint16_t,
    const void*)
{
    return {0};
}

mtd::MockXkb::MockXkb()
{
    using namespace testing;
    assert(global_mock == nullptr && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, xkb_x11_get_core_keyboard_device_id(_))
    .WillByDefault(Return(12));
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
