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

#include "src/platforms/mesa/include/native_buffer.h"
#include "src/platforms/mesa/client/native_surface.h"
#include "mir/client_buffer.h"
#include "mir/test/doubles/mock_egl_native_surface.h"
#include "mir/test/doubles/mock_client_buffer.h"
#include "mir/test/fake_shared.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mcl = mir::client;
namespace mclg = mcl::mesa;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

class MesaClientNativeSurfaceTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;
        surf_params.width = 530;
        surf_params.height = 715;
        surf_params.pixel_format = mir_pixel_format_abgr_8888;

        ON_CALL(mock_surface, get_parameters())
            .WillByDefault(Return(surf_params));
        ON_CALL(mock_surface, get_current_buffer())
            .WillByDefault(Return(
                mt::fake_shared(mock_buffer)));
        ON_CALL(mock_buffer, native_buffer_handle())
            .WillByDefault(Return(
                mt::fake_shared(native_buffer)));
    }

    MirWindowParameters surf_params;
    const char* error_msg = "thrown as part of test";
    mg::mesa::NativeBuffer native_buffer;
    testing::NiceMock<mtd::MockClientBuffer> mock_buffer;
    testing::NiceMock<mtd::MockEGLNativeSurface> mock_surface;
    mclg::NativeSurface native_surface{&mock_surface};
};

TEST_F(MesaClientNativeSurfaceTest, basic_parameters)
{
    MirWindowParameters params;
    EXPECT_EQ(MIR_MESA_TRUE, native_surface.surface_get_parameters(&native_surface, &params));
    EXPECT_EQ(surf_params.width, params.width);
    EXPECT_EQ(surf_params.height, params.height);
    EXPECT_EQ(surf_params.pixel_format, params.pixel_format);
}

TEST_F(MesaClientNativeSurfaceTest, first_advance_skips_request)
{   // Verify the workaround for LP: #1281938 is functioning, until it's
    // fixed in Mesa and we can remove it from Mir.
    
    using namespace testing;
    MirBufferPackage buffer_package;
    EXPECT_CALL(mock_surface, swap_buffers_sync())
        .Times(0);
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1);

    EXPECT_EQ(MIR_MESA_TRUE, native_surface.surface_advance_buffer(&native_surface, &buffer_package));
}

TEST_F(MesaClientNativeSurfaceTest, basic_advance)
{
    using namespace testing;
    MirBufferPackage buffer_package;

    InSequence seq;
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1);
    EXPECT_CALL(mock_surface, swap_buffers_sync())
        .Times(1);
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1);

    EXPECT_EQ(MIR_MESA_TRUE, native_surface.surface_advance_buffer(&native_surface, &buffer_package));
    EXPECT_EQ(MIR_MESA_TRUE, native_surface.surface_advance_buffer(&native_surface, &buffer_package));
}

TEST_F(MesaClientNativeSurfaceTest, swapinterval_request)
{
    using namespace testing;

    Sequence seq;
    EXPECT_CALL(mock_surface, request_and_wait_for_configure(mir_window_attrib_swapinterval,0))
        .InSequence(seq);
    EXPECT_CALL(mock_surface, request_and_wait_for_configure(mir_window_attrib_swapinterval,1))
        .InSequence(seq);

    EXPECT_EQ(MIR_MESA_TRUE, native_surface.set_swapinterval(0));
    EXPECT_EQ(MIR_MESA_TRUE, native_surface.set_swapinterval(1));
}

TEST_F(MesaClientNativeSurfaceTest, swapinterval_unsupported_request)
{
    EXPECT_EQ(MIR_MESA_FALSE, native_surface.set_swapinterval(-1));
    EXPECT_EQ(MIR_MESA_TRUE, native_surface.set_swapinterval(0));
    EXPECT_EQ(MIR_MESA_TRUE, native_surface.set_swapinterval(1));
    EXPECT_EQ(MIR_MESA_FALSE, native_surface.set_swapinterval(2));
}

TEST_F(MesaClientNativeSurfaceTest, returns_error_on_advance_buffer_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_surface, get_current_buffer())
        .WillOnce(Throw(std::runtime_error(error_msg)));

    MirBufferPackage buffer_package;
    EXPECT_EQ(MIR_MESA_FALSE, native_surface.advance_buffer(&buffer_package));
}

TEST_F(MesaClientNativeSurfaceTest, returns_error_on_get_parameters_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_surface, get_parameters())
        .WillOnce(Throw(std::runtime_error(error_msg)));

    MirWindowParameters surface_params;
    EXPECT_EQ(MIR_MESA_FALSE, native_surface.get_parameters(&surface_params));
}

TEST_F(MesaClientNativeSurfaceTest, returns_error_on_set_swap_interval_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_surface, request_and_wait_for_configure(_,_))
        .WillOnce(Throw(std::runtime_error(error_msg)));

    EXPECT_EQ(MIR_MESA_FALSE, native_surface.set_swapinterval(0));
}

TEST_F(MesaClientNativeSurfaceTest, null_native_surface_returns_error)
{
    using namespace testing;

    MirWindowParameters params;
    MirBufferPackage buffer_package;

    mclg::NativeSurface null_native_surface{nullptr};

    EXPECT_EQ(MIR_MESA_FALSE,
        null_native_surface.surface_get_parameters(&null_native_surface, &params));
    EXPECT_EQ(MIR_MESA_FALSE,
        null_native_surface.surface_advance_buffer(&null_native_surface, &buffer_package));
    EXPECT_EQ(MIR_MESA_FALSE,
        null_native_surface.surface_set_swapinterval(&null_native_surface, 1));
}

TEST_F(MesaClientNativeSurfaceTest, native_surface_after_null_returns_success)
{
    using namespace testing;

    MirWindowParameters params;
    MirBufferPackage buffer_package;

    mclg::NativeSurface native_surface{nullptr};

    native_surface.use_native_surface(&mock_surface);
    EXPECT_EQ(MIR_MESA_TRUE,
        native_surface.surface_get_parameters(&native_surface, &params));
    EXPECT_EQ(MIR_MESA_TRUE,
            native_surface.surface_advance_buffer(&native_surface, &buffer_package));
    EXPECT_EQ(MIR_MESA_TRUE,
            native_surface.surface_set_swapinterval(&native_surface, 1));
}
