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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/client_platform.h"
#include "mir/shared_library.h"
#include "src/platforms/mesa/client/mesa_native_display_container.h"
#include "mir_test_framework/client_platform_factory.h"
#include "mir_test_doubles/mock_client_surface.h"

#include "mir_toolkit/mesa/native_display.h"

#include <gtest/gtest.h>

namespace mcl = mir::client;
namespace mclm = mir::client::mesa;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

TEST(MesaClientPlatformTest, egl_native_display_is_valid_until_released)
{
    auto platform = mtf::create_mesa_client_platform();
    auto platform_lib = mtf::get_platform_library();

    MirMesaEGLNativeDisplay* nd;
    {
        std::shared_ptr<EGLNativeDisplayType> native_display = platform->create_egl_native_display();

        nd = reinterpret_cast<MirMesaEGLNativeDisplay*>(*native_display);
        auto validate = platform_lib->load_function<MirBool(*)(MirMesaEGLNativeDisplay*)>("mir_client_mesa_egl_native_display_is_valid");
        EXPECT_EQ(MIR_MESA_TRUE, validate(nd));
    }
    EXPECT_EQ(MIR_MESA_FALSE, mclm::mir_client_mesa_egl_native_display_is_valid(nd));
}
