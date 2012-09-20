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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/graphics/gbm/gbm_platform.h"
#include "mir/graphics/gbm/gbm_buffer.h"
#include "mir/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/compositor/buffer_ipc_package.h"

#include "mock_drm.h"
#include "mock_gbm.h"

#include <gbm.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace geom=mir::geometry;

class GBMGraphicBufferBasic : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        auto platform = std::make_shared<mgg::GBMPlatform>();
        allocator.reset(new mgg::GBMBufferAllocator(platform));

        width = geom::Width(300);
        height = geom::Height(200);
        pf = geom::PixelFormat::rgba_8888;

        ON_CALL(mock_gbm, gbm_bo_get_width(_))
        .WillByDefault(Return(width.as_uint32_t()));

        ON_CALL(mock_gbm, gbm_bo_get_height(_))
        .WillByDefault(Return(height.as_uint32_t()));

        ON_CALL(mock_gbm, gbm_bo_get_format(_))
        .WillByDefault(Return(GBM_BO_FORMAT_ARGB8888));

        ON_CALL(mock_gbm, gbm_bo_get_stride(_))
        .WillByDefault(Return(4 * width.as_uint32_t()));
    }

    ::testing::NiceMock<mgg::MockDRM> mock_drm;
    ::testing::NiceMock<mgg::MockGBM> mock_gbm;
    std::unique_ptr<mgg::GBMBufferAllocator> allocator;

    // Defaults
    geom::PixelFormat pf;
    geom::Width width;
    geom::Height height;
};

TEST_F(GBMGraphicBufferBasic, dimensions_test)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    std::unique_ptr<mc::Buffer> buffer = allocator->alloc_buffer(width, height, pf);
    ASSERT_EQ(width, buffer->width());
    ASSERT_EQ(height, buffer->height());
}

TEST_F(GBMGraphicBufferBasic, buffer_has_expected_pixel_format)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    std::unique_ptr<mc::Buffer> buffer(allocator->alloc_buffer(width, height, pf));
    ASSERT_EQ(pf, buffer->pixel_format());
}

TEST_F(GBMGraphicBufferBasic, stride_has_sane_value)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    // RGBA 8888 cannot take less than 4 bytes
    // TODO: is there a *maximum* sane value for stride?
    geom::Stride minimum(width.as_uint32_t() * 4);

    std::unique_ptr<mc::Buffer> buffer(allocator->alloc_buffer(width, height, pf));

    ASSERT_LE(minimum, buffer->stride());
}

TEST_F(GBMGraphicBufferBasic, buffer_ipc_package_has_correct_size)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_get_handle(_))
            .Times(Exactly(1));

    auto buffer = allocator->alloc_buffer(width, height, pf);
    auto ipc_package = buffer->get_ipc_package();
    ASSERT_TRUE(ipc_package->ipc_fds.empty());
    ASSERT_EQ(size_t(1), ipc_package->ipc_data.size());
}

TEST_F(GBMGraphicBufferBasic, buffer_ipc_package_contains_correct_handle)
{
    using namespace testing;

    gbm_bo_handle mock_handle;
    mock_handle.u32 = 0xdeadbeef;

    EXPECT_CALL(mock_gbm, gbm_bo_get_handle(_))
            .Times(Exactly(1))
            .WillOnce(Return(mock_handle));

    auto buffer = allocator->alloc_buffer(width, height, pf);
    auto ipc_package = buffer->get_ipc_package();
    ASSERT_EQ(mock_handle.u32, static_cast<uint32_t>(ipc_package->ipc_data[0]));
}
