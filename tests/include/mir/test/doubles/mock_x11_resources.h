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

#ifndef MIR_TEST_DOUBLES_MOCK_X11_RESOURCES_
#define MIR_TEST_DOUBLES_MOCK_X11_RESOURCES_

#include "src/platforms/x11/x11_resources.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockXCBConnection : public mir::X::XCBConnection
{
public:
    MockXCBConnection()
    {
        ON_CALL(*this, screen())
            .WillByDefault(testing::Return(&fake_screen));
    }

    MOCK_METHOD(int, has_error, (), (const, override));
    MOCK_METHOD(int, get_file_descriptor, (), (const, override));
    MOCK_METHOD(xcb_generic_event_t*, poll_for_event, (), (const, override));
    MOCK_METHOD(xcb_screen_t*, screen, (), (const, override));
    MOCK_METHOD(xcb_atom_t, intern_atom, (std::string const& name), (const, override));
    MOCK_METHOD(double, get_output_refresh_rate, (), (const, override));
    MOCK_METHOD(xcb_query_extension_reply_t const*, get_extension_data, (xcb_extension_t *ext), (const, override));
    MOCK_METHOD(uint32_t, generate_id, (), (const, override));
    MOCK_METHOD(MirPixelFormat, default_pixel_format, (), (const, override));
    MOCK_METHOD(void, create_window, (xcb_window_t window,
        int16_t x, int16_t y,
        uint32_t value_mask,
        const void* value_list), (const, override));
    MOCK_METHOD(void, change_property, (xcb_window_t window,
        xcb_atom_t property_atom,
        xcb_atom_t type_atom,
        uint8_t format,
        size_t length,
        void const* data), (const, override));
    MOCK_METHOD(void, map_window, (xcb_window_t window), (const, override));
    MOCK_METHOD(void, destroy_window, (xcb_window_t window), (const, override));
    MOCK_METHOD(void, flush, (), (const, override));
    MOCK_METHOD(xcb_connection_t*, connection, (), (const, override));

    xcb_screen_t fake_screen{
        0, 0, 0, 0, 0,
        640, // width_in_pixels;
        480, // height_in_pixels;
        400, // width_in_millimeters;
        300, // height_in_millimeters;
        0, 0, 0, 0, 0, 0, 0};
};

class MockX11Resources : public mir::X::X11Resources
{
public:
    MockX11Resources()
        : X11Resources{std::make_unique<testing::NiceMock<MockXCBConnection>>(), nullptr}
    {
    }
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_X11_RESOURCES_
