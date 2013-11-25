/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/client/client_platform.h"
#include "src/client/native_client_platform_factory.h"
#include "src/client/mir_client_surface.h"
#include "mir_test_doubles/mock_client_context.h"
#include "mir_test_doubles/mock_client_surface.h"

#ifdef ANDROID
#include "mir_test_doubles/mock_android_hw.h"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl=mir::client;
namespace mtd = mir::test::doubles;

class ClientPlatformTest : public ::testing::Test
{
#ifdef ANDROID
    testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
#endif
};

TEST_F(ClientPlatformTest, platform_name)
{
    mtd::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    auto platform = factory.create_client_platform(&context);

#ifdef ANDROID
    auto type = mir_platform_type_android;
#else
    auto type = mir_platform_type_gbm;
#endif
    EXPECT_EQ(type, platform->platform_type());
}

TEST_F(ClientPlatformTest, platform_creates)
{
    mtd::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    auto platform = factory.create_client_platform(&context);
    auto buffer_factory = platform->create_buffer_factory();
    EXPECT_NE(buffer_factory.get(), (mcl::ClientBufferFactory*) NULL);
}

TEST_F(ClientPlatformTest, platform_creates_native_window)
{
    mtd::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    auto platform = factory.create_client_platform(&context);
    auto mock_client_surface = std::make_shared<mtd::MockClientSurface>();
    auto native_window = platform->create_egl_native_window(mock_client_surface.get());
    EXPECT_NE(*native_window, (EGLNativeWindowType) NULL);
}

TEST_F(ClientPlatformTest, platform_creates_egl_native_display)
{
    mtd::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    auto platform = factory.create_client_platform(&context);
    auto native_display = platform->create_egl_native_display();
    EXPECT_NE(nullptr, native_display.get());
}
