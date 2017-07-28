/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_CLIENT_PLATFOM_H_
#define MIR_TEST_DOUBLES_MOCK_CLIENT_PLATFOM_H_

#include "stub_client_buffer_factory.h"
#include "mir/client/client_platform.h"
#include "mir/client/client_platform_factory.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockClientPlatform : public client::ClientPlatform
{
    MockClientPlatform()
    {
        auto native_window = std::make_shared<EGLNativeWindowType>();
        *native_window = reinterpret_cast<EGLNativeWindowType>(this);

        ON_CALL(*this, create_buffer_factory())
            .WillByDefault(testing::Return(std::make_shared<StubClientBufferFactory>()));
        ON_CALL(*this, create_egl_native_window(testing::_))
            .WillByDefault(testing::Return(native_window));
    }

    void set_client_context(client::ClientContext* ctx)
    {
        client_context = ctx;
    }

    void populate(MirPlatformPackage& pkg) const override
    {
        client_context->populate_server_package(pkg);
    }

    MOCK_CONST_METHOD1(convert_native_buffer, MirNativeBuffer*(mir::graphics::NativeBuffer*));
    MOCK_CONST_METHOD0(platform_type, MirPlatformType());
    MOCK_METHOD1(platform_operation, MirPlatformMessage*(MirPlatformMessage const*));
    MOCK_METHOD0(create_buffer_factory, std::shared_ptr<client::ClientBufferFactory>());
    MOCK_METHOD2(use_egl_native_window, void(std::shared_ptr<void>, client::EGLNativeSurface*));
    MOCK_METHOD1(create_egl_native_window, std::shared_ptr<void>(client::EGLNativeSurface*));
    MOCK_METHOD0(create_egl_native_display, std::shared_ptr<EGLNativeDisplayType>());
    MOCK_CONST_METHOD2(get_egl_pixel_format, MirPixelFormat(EGLDisplay, EGLConfig));
    MOCK_METHOD2(request_interface, void*(char const*, int));
    MOCK_CONST_METHOD1(native_format_for, uint32_t(MirPixelFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MOCK_CONST_METHOD2(native_flags_for, uint32_t(MirBufferUsage, mir::geometry::Size));
#pragma GCC diagnostic pop
    client::ClientContext* client_context = nullptr;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_CLIENT_PLATFOM_H_ */
