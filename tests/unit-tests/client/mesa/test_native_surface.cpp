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

#include "src/client/mesa/native_surface.h"
#include "src/client/client_buffer.h"
#include "mir_test_doubles/mock_client_surface.h"

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
    testing::NiceMock<mtd::MockClientSurface> mock_surface;
};

TEST_F(MesaClientNativeSurfaceTest, basic_parameters)
{
    mclg::NativeSurface interpreter(mock_surface);

    MirSurfaceParameters params;
    interpreter.surface_get_parameters(&interpreter, &params);
    EXPECT_EQ(surf_params.width, params.width);
    EXPECT_EQ(surf_params.height, params.height);
    EXPECT_EQ(surf_params.pixel_format, params.pixel_format);
}

TEST_F(MesaClientNativeSurfaceTest, basic_advance)
{
    using namespace testing;
    MirBufferPackage buffer_package;
    EXPECT_CALL(mock_surface, request_and_wait_for_next_buffer())
        .Times(1);
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1);

    mclg::NativeSurface interpreter(mock_surface);
    interpreter.surface_advance_buffer(&interpreter, &buffer_package);
}

TEST_F(MesaClientNativeSurfaceTest, swapinterval_request)
{
    using namespace testing;

    Sequence seq;
    EXPECT_CALL(mock_surface, request_and_wait_for_configure(mir_surface_attrib_swapinterval,0))
        .InSequence(seq);
    EXPECT_CALL(mock_surface, request_and_wait_for_configure(mir_surface_attrib_swapinterval,1))
        .InSequence(seq);

    mclg::NativeSurface interpreter(mock_surface);
    interpreter.set_swapinterval(0);
    interpreter.set_swapinterval(1);
}

TEST_F(MesaClientNativeSurfaceTest, swapinterval_unsupported_request)
{
    mclg::NativeSurface interpreter(mock_surface);
    EXPECT_EQ(MIR_MESA_FALSE, interpreter.set_swapinterval(-1));
    EXPECT_EQ(MIR_MESA_TRUE, interpreter.set_swapinterval(0));
    EXPECT_EQ(MIR_MESA_TRUE, interpreter.set_swapinterval(1));
    EXPECT_EQ(MIR_MESA_FALSE, interpreter.set_swapinterval(2));
}
