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

#include "src/client/gbm/mesa_native_display_container.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mesa/native_display.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

namespace mtd = mir::test::doubles;
namespace mclg = mir::client::gbm;

namespace
{

struct MesaNativeDisplayContainerSetup : public testing::Test
{
    void SetUp()
    {
        connection = mock_client_library.a_connection();
        surface = mock_client_library.a_surface();
        
        container = std::make_shared<mclg::MesaNativeDisplayContainer>();
    }
    
    mtd::MockMesaEGLClientLibrary mock_client_library;
    MirConnection* connection;
    MirSurface* surface;
    std::shared_ptr<mclg::MesaNativeDisplayContainer> container;
};

}

TEST_F(MesaNativeDisplayContainerSetup, valid_displays_come_from_factory)
{
    using namespace ::testing;
    
    auto display = container->create(connection);
    EXPECT_TRUE(container->validate(display));
    
    MirEGLNativeDisplayType invalid_native_display;
    EXPECT_FALSE(container->validate(&invalid_native_display));
}

TEST_F(MesaNativeDisplayContainerSetup, releasing_displays_invalidates_address)
{
    using namespace ::testing;
    
    auto display = container->create(connection);
    EXPECT_TRUE(container->validate(display));
    container->release(display);
    EXPECT_FALSE(container->validate(display));
}

TEST_F(MesaNativeDisplayContainerSetup, display_get_platform)
{
    using namespace ::testing;

    auto display = container->create(connection);
    auto native_display = reinterpret_cast<mir_toolkit::MirMesaEGLNativeDisplay*>(display);

    MirPlatformPackage platform_package;
    EXPECT_CALL(mock_client_library, connection_get_platform(connection, &platform_package)).Times(1);
    native_display->display_get_platform(native_display, &platform_package);
}

TEST_F(MesaNativeDisplayContainerSetup, surface_get_current_buffer)
{
    using namespace ::testing;

    auto display = container->create(connection);
    auto native_display = reinterpret_cast<mir_toolkit::MirMesaEGLNativeDisplay*>(display);

    MirBufferPackage buffer_package;
    EXPECT_CALL(mock_client_library, surface_get_current_buffer(surface, &buffer_package)).Times(1);
    native_display->surface_get_current_buffer(native_display, (MirEGLNativeWindowType)surface, &buffer_package);
}

TEST_F(MesaNativeDisplayContainerSetup, surface_get_parameters)
{
    using namespace ::testing;

    auto display = container->create(connection);
    auto native_display = reinterpret_cast<mir_toolkit::MirMesaEGLNativeDisplay*>(display);

    MirSurfaceParameters surface_parameters;
    EXPECT_CALL(mock_client_library, surface_get_parameters(surface, &surface_parameters)).Times(1);
    native_display->surface_get_parameters(native_display, (MirEGLNativeWindowType)surface, &surface_parameters);
}

TEST_F(MesaNativeDisplayContainerSetup, surface_advance_buffer)
{
    using namespace ::testing;

    auto display = container->create(connection);
    auto native_display = reinterpret_cast<mir_toolkit::MirMesaEGLNativeDisplay*>(display);

    {
        InSequence seq;
        EXPECT_CALL(mock_client_library, surface_next_buffer(surface, _, _)).Times(1);
        EXPECT_CALL(mock_client_library, wait_for(_)).Times(1);
    }
    native_display->surface_advance_buffer(native_display, (MirEGLNativeWindowType)surface);
}
