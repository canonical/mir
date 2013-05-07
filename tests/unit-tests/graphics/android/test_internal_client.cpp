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
#include "src/server/graphics/android/internal_client.h"
#include <system/window.h>
#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mc=mir::compositor;
namespace mga=mir::graphics::android;

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
    mga::InternalClient client(surface);
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, client.egl_native_display());
}

TEST(InternalClient, native_window)
{
    auto surface = std::make_shared<StubSurface>();
    mga::InternalClient client(surface);
    ANativeWindow* native_window = static_cast<ANativeWindow*>(client.egl_native_window());

    /* check for basic window sanity */
    ASSERT_NE(nullptr, native_window);
    EXPECT_NE(nullptr, native_window->queueBuffer);
    EXPECT_NE(nullptr, native_window->dequeueBuffer);
    EXPECT_NE(nullptr, native_window->query);
    EXPECT_NE(nullptr, native_window->perform);
}
