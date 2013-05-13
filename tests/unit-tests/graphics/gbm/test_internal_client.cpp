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
#include "mir_test_doubles/stub_surface.h"

#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mc=mir::compositor;
namespace mgg=mir::graphics::gbm;
namespace mtd=mir::test::doubles;

TEST(InternalClient, native_display)
{
    auto stub_window = std::make_shared<mtd::StubSurface>();
    auto stub_display = std::make_shared<MirMesaEGLNativeDisplay>();
    mgg::InternalClient client(stub_display, stub_window);

    auto native_display = client.egl_native_display();
    auto native_window = client.egl_native_window();

    EXPECT_EQ(reinterpret_cast<EGLNativeDisplayType>(stub_display.get()), native_display);
    EXPECT_EQ(reinterpret_cast<EGLNativeWindowType>(stub_window.get()), native_window);
}
