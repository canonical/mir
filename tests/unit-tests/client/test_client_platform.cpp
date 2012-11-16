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

#include "mir_client/client_platform.h"
#include "mir_client/native_client_platform_factory.h"
#include "mir_client/mir_client_surface.h"
#include "mock_client_context.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl=mir::client;

struct MockClientSurface : public mcl::ClientSurface
{
    MOCK_CONST_METHOD0(get_parameters, MirSurfaceParameters());
    MOCK_METHOD0(get_current_buffer, std::shared_ptr<mcl::ClientBuffer>());
    MOCK_METHOD2(next_buffer, MirWaitHandle*(mir_surface_lifecycle_callback, void*));
}; 

TEST(ClientPlatformTest, platform_creates )
{
    mcl::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    auto platform = factory.create_client_platform(&context);
    auto depository = platform->create_platform_depository(); 
    EXPECT_NE( depository.get(), (mcl::ClientBufferDepository*) NULL);
}

TEST(ClientPlatformTest, platform_creates_native_window )
{
    mcl::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    MockClientSurface surface;
    auto platform = factory.create_client_platform(&context);
    auto native_window = platform->create_egl_window(&surface);
    EXPECT_NE( native_window, (EGLNativeWindowType) NULL);
}

TEST(ClientPlatformTest, platform_creates_egl_native_display)
{
    mcl::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    auto platform = factory.create_client_platform(&context);
    auto native_display = platform->create_egl_native_display();
    EXPECT_NE(nullptr, native_display.get());
}
