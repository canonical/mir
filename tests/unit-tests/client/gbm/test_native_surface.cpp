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

#include "src/client/gbm/gbm_native_surface.h"
#include "src/client/client_buffer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl=mir::client;
namespace mcl=mir::client;
namespace mclg=mir::client::gbm;
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
    MOCK_CONST_METHOD0(pixel_format, geom::PixelFormat());

    MOCK_CONST_METHOD0(age, uint32_t());
    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_METHOD0(increment_age, void());

    MOCK_CONST_METHOD0(native_buffer_handle, std::shared_ptr<MirNativeBuffer>());
};

struct MockMirSurface : public mcl::ClientSurface
{
    MockMirSurface(MirSurfaceParameters params)
     : params(params)
    {
        using namespace testing;
        ON_CALL(*this, get_parameters())
            .WillByDefault(Return(params));
        ON_CALL(*this, get_current_buffer())
            .WillByDefault(Return(
                std::make_shared<NiceMock<MockClientBuffer>>()));
    }

    MOCK_CONST_METHOD0(get_parameters, MirSurfaceParameters());
    MOCK_METHOD0(get_current_buffer, std::shared_ptr<mcl::ClientBuffer>());
    MOCK_METHOD2(next_buffer, MirWaitHandle*(mir_surface_lifecycle_callback callback, void * context));

    MirSurfaceParameters params;
};
}

class GBMInterpreterTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;
        surf_params.width = 530;
        surf_params.height = 715;
        surf_params.pixel_format = mir_pixel_format_abgr_8888;

        buffer = std::make_shared<MirBufferPackage>();
    }

    std::shared_ptr<MirBufferPackage> buffer;
    MirSurfaceParameters surf_params;
};

TEST_F(GBMInterpreterTest, basic_parameters)
{
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mclg::GBMNativeSurface interpreter(mock_surface);

    MirSurfaceParameters params;
    interpreter.surface_get_parameters(&interpreter, &params);
}

TEST_F(GBMInterpreterTest, basic_advance)
{
    using namespace testing;
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};

    mclg::GBMNativeSurface interpreter(mock_surface);

    EXPECT_CALL(mock_surface, next_buffer(_,_))
        .Times(1);
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1);

    MirBufferPackage buffer_package;
    interpreter.surface_advance_buffer(&interpreter, &buffer_package);
}
