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

#include "src/platform/graphics/mesa/internal_client.h"
#include "mir/graphics/internal_surface.h"
#include "mir_toolkit/mesa/native_display.h"
#include "src/platform/graphics/mesa/internal_native_surface.h"

#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mg = mir::graphics;
namespace mgm=mir::graphics::mesa;

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

    void swap_buffers(mg::Buffer*&)
    {
    }
};
}

TEST(InternalClient, native_display_sanity)
{
    auto stub_display = std::make_shared<MirMesaEGLNativeDisplay>();
    mgm::InternalClient client(stub_display);

    auto native_display = client.egl_native_display();
    EXPECT_EQ(reinterpret_cast<EGLNativeDisplayType>(stub_display.get()), native_display);
}

TEST(InternalClient, native_surface_sanity)
{
    auto stub_display = std::make_shared<MirMesaEGLNativeDisplay>();
    mgm::InternalClient client(stub_display);

    auto stub_window = std::make_shared<StubInternalSurface>();
    auto native_window = static_cast<mgm::InternalNativeSurface*>(client.egl_native_window(stub_window));

    ASSERT_NE(nullptr, native_window->surface_advance_buffer);
    ASSERT_NE(nullptr, native_window->surface_get_parameters);
    ASSERT_NE(nullptr, native_window->surface_set_swapinterval);
}
