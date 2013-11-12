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

#include "src/server/graphics/android/internal_client.h"
#include "mir/graphics/internal_surface.h"

#include <system/window.h>
#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

namespace
{
class StubInternalSurface : public mg::InternalSurface
{
public:
    geom::Size size() const
    {
        return geom::Size();
    }

    MirPixelFormat pixel_format() const
    {
        return MirPixelFormat();
    }

    std::shared_ptr<mg::Buffer> advance_client_buffer()
    {
        return std::shared_ptr<mg::Buffer>();
    }
};
}

TEST(InternalClient, native_display)
{
    auto surface = std::make_shared<StubInternalSurface>();
    mga::InternalClient client;
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, client.egl_native_display());
}

TEST(InternalClient, native_window)
{
    auto surface = std::make_shared<StubInternalSurface>();
    mga::InternalClient client;
    ANativeWindow* native_window = static_cast<ANativeWindow*>(client.egl_native_window(surface));

    /* check for basic window sanity */
    ASSERT_NE(nullptr, native_window);
    EXPECT_NE(nullptr, native_window->queueBuffer);
    EXPECT_NE(nullptr, native_window->dequeueBuffer);
    EXPECT_NE(nullptr, native_window->query);
    EXPECT_NE(nullptr, native_window->perform);
}
