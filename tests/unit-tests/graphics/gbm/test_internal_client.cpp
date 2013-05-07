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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/frontend/surface.h"
#include "mir_toolkit/mesa/native_display.h"
#include "src/server/graphics/gbm/internal_client.h"
#include "mir_test_doubles/stub_platform.h"

#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mc=mir::compositor;
namespace mgg=mir::graphics::gbm;
namespace mtd=mir::test::doubles;

namespace
{
class StubSurface : public mir::frontend::Surface
{
    void destroy()
    {
    }
    void force_requests_to_complete()
    {
    }
    geom::Size size() const
    {
        return geom::Size{geom::Width{4},geom::Height{2}};
    }
    geom::PixelFormat pixel_format() const
    {
        return geom::PixelFormat::xbgr_8888;
    }
    void advance_client_buffer()
    {
    }
    std::shared_ptr<mc::Buffer> client_buffer() const
    {
        return std::shared_ptr<mc::Buffer>();
    }
    bool supports_input() const
    {
        return false;
    }
    int client_input_fd() const
    {
        return 5;
    }
    int configure(MirSurfaceAttrib, int)
    {
        return 218181;
    }
}; 
}

TEST(InternalClient, native_display)
{
    auto surface = std::make_shared<StubSurface>();
    auto platform = std::make_shared<mtd::StubPlatform>();
    mgg::InternalClient client(platform, surface);

    auto native_display = client.egl_native_display();
    MirMesaEGLNativeDisplay* disp = reinterpret_cast<MirMesaEGLNativeDisplay*>(native_display); 

    ASSERT_NE(nullptr, disp);
    EXPECT_NE(nullptr, disp->display_get_platform);
    EXPECT_NE(nullptr, disp->surface_get_current_buffer);
    EXPECT_NE(nullptr, disp->surface_get_parameters);
    EXPECT_NE(nullptr, disp->surface_advance_buffer);
    EXPECT_NE(nullptr, disp->context);
}

TEST(InternalClient, native_window)
{
    auto surface = std::make_shared<StubSurface>();
    auto platform = std::make_shared<mtd::StubPlatform>();
    mgg::InternalClient client(platform, surface);

    auto native_display = client.egl_native_window();

    //in mesa, mf::Surface is the EGLNativeWindowType
    EXPECT_EQ(native_display, surface.get());
}
