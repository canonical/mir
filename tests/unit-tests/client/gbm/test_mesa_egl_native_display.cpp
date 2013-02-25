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

#include "mock_mesa_egl_client_library.h"

#include "src/client/gbm/gbm_client_egl_native_display.h"

#include "mir_client/mir_client_library.h"
#include "mir_client/mir_client_library_mesa_egl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
namespace mclg = mir::client::gbm;

namespace
{

struct MesaEGLNativeDisplaySetup : public testing::Test
{
    void SetUp()
    {
        connection = mock_client_library.a_connection();
        surface = mock_client_library.a_surface();
        native_display = mclg::EGL::create_native_display(connection);
    }
    
    mtd::MockMesaEGLClientLibrary mock_client_library;
    MirConnection *connection;
    MirSurface *surface;
    MirMesaEGLNativeDisplay *native_display;
};

}

TEST_F(MesaEGLNativeDisplaySetup, valid_displays_come_from_factory)
{
    using namespace ::testing;

    EXPECT_TRUE(mir_mesa_egl_native_display_is_valid(native_display));
    
    MirMesaEGLNativeDisplay invalid_native_display;
    EXPECT_FALSE(mir_mesa_egl_native_display_is_valid(&invalid_native_display));
}

TEST_F(MesaEGLNativeDisplaySetup, releasing_displays_invalidates_address)
{
    using namespace ::testing;
    
    EXPECT_TRUE(mir_mesa_egl_native_display_is_valid(native_display));
    mclg::EGL::release_native_display(native_display);
    EXPECT_FALSE(mir_mesa_egl_native_display_is_valid(native_display));
}

TEST_F(MesaEGLNativeDisplaySetup, display_get_platform)
{
    using namespace ::testing;

    MirPlatformPackage platform_package;

    EXPECT_CALL(mock_client_library, connection_get_platform(connection, &platform_package)).Times(1);
    native_display->display_get_platform(native_display, &platform_package);
}

TEST_F(MesaEGLNativeDisplaySetup, surface_get_current_buffer)
{
    using namespace ::testing;

    MirBufferPackage buffer_package;
    
    EXPECT_CALL(mock_client_library, surface_get_current_buffer(surface, &buffer_package)).Times(1);
    native_display->surface_get_current_buffer(native_display, (EGLNativeWindowType)surface, &buffer_package);
}

TEST_F(MesaEGLNativeDisplaySetup, surface_get_parameters)
{
    using namespace ::testing;

    MirSurfaceParameters surface_parameters;
    
    EXPECT_CALL(mock_client_library, surface_get_parameters(surface, &surface_parameters)).Times(1);
    native_display->surface_get_parameters(native_display, (EGLNativeWindowType)surface, &surface_parameters);
}

TEST_F(MesaEGLNativeDisplaySetup, surface_advance_buffer)
{
    using namespace ::testing;

    EXPECT_CALL(mock_client_library, surface_next_buffer(surface, _, _)).Times(1);
    native_display->surface_advance_buffer(native_display, (EGLNativeWindowType)surface);
}
