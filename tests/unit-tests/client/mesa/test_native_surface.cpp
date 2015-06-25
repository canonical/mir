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

#include "src/platforms/mesa/client/native_surface.h"
#include "mir/client_buffer.h"
#include "mir/test/doubles/mock_egl_native_surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtd=mir::test::doubles;
namespace mcl=mir::client;
namespace mcl=mir::client;
namespace mclg=mir::client::mesa;
namespace geom=mir::geometry;

namespace
{

struct MockClientBuffer : public mcl::ClientBuffer
{
    MockClientBuffer()
    {
        ON_CALL(*this, native_buffer_handle())
            .WillByDefault(testing::Return(std::make_shared<MirBufferPackage>()));
    }
    ~MockClientBuffer() noexcept {}

    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, MirPixelFormat());

    MOCK_CONST_METHOD0(age, uint32_t());
    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_METHOD0(increment_age, void());
    MOCK_METHOD1(update_from, void(MirBufferPackage const&));
    MOCK_METHOD1(fill_update_msg, void(MirBufferPackage&));
    MOCK_CONST_METHOD0(native_buffer_handle, std::shared_ptr<MirNativeBuffer>());
};

}

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
                std::make_shared<NiceMock<MockClientBuffer>>()));
    }

    MirSurfaceParameters surf_params;
    testing::NiceMock<mtd::MockEGLNativeSurface> mock_surface;
    mclg::NativeSurface native_surface{mock_surface};
};

TEST_F(MesaClientNativeSurfaceTest, basic_parameters)
{
    MirSurfaceParameters params;
    native_surface.surface_get_parameters(&native_surface, &params);
    EXPECT_EQ(surf_params.width, params.width);
    EXPECT_EQ(surf_params.height, params.height);
    EXPECT_EQ(surf_params.pixel_format, params.pixel_format);
}

TEST_F(MesaClientNativeSurfaceTest, first_advance_skips_request)
{   // Verify the workaround for LP: #1281938 is functioning, until it's
    // fixed in Mesa and we can remove it from Mir.
    
    using namespace testing;
    MirBufferPackage buffer_package;
    EXPECT_CALL(mock_surface, request_and_wait_for_next_buffer())
        .Times(0);
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1);

    native_surface.surface_advance_buffer(&native_surface, &buffer_package);
}

TEST_F(MesaClientNativeSurfaceTest, basic_advance)
{
    using namespace testing;
    MirBufferPackage buffer_package;

    InSequence seq;
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1);
    EXPECT_CALL(mock_surface, request_and_wait_for_next_buffer())
        .Times(1);
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1);

    native_surface.surface_advance_buffer(&native_surface, &buffer_package);
    native_surface.surface_advance_buffer(&native_surface, &buffer_package);
}

TEST_F(MesaClientNativeSurfaceTest, swapinterval_request)
{
    using namespace testing;

    Sequence seq;
    EXPECT_CALL(mock_surface, request_and_wait_for_configure(mir_surface_attrib_swapinterval,0))
        .InSequence(seq);
    EXPECT_CALL(mock_surface, request_and_wait_for_configure(mir_surface_attrib_swapinterval,1))
        .InSequence(seq);

    native_surface.set_swapinterval(0);
    native_surface.set_swapinterval(1);
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
        .WillOnce(Throw(std::runtime_error("")));

    MirBufferPackage buffer_package;
    EXPECT_THAT(native_surface.advance_buffer(&buffer_package), Eq(MIR_MESA_FALSE));
}

TEST_F(MesaClientNativeSurfaceTest, returns_error_on_get_parameters_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_surface, get_parameters())
        .WillOnce(Throw(std::runtime_error("")));

    MirSurfaceParameters surface_params;
    EXPECT_THAT(native_surface.get_parameters(&surface_params), Eq(MIR_MESA_FALSE));
}

TEST_F(MesaClientNativeSurfaceTest, returns_error_on_set_swap_interval_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_surface, request_and_wait_for_configure(_,_))
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THAT(native_surface.set_swapinterval(0), Eq(MIR_MESA_FALSE));
}
