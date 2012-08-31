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

#include "mir/graphics/gbm/gbm_buffer.h"
#include "mir/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/graphics/graphic_alloc_adaptor.h"

#include "mock_gbm_buffer_allocator.h"

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
        mocker = std::shared_ptr<mgg::MockGBMDeviceCreator>(new mgg::MockGBMDeviceCreator);
        allocator = mocker->create_gbm_allocator();

        width = geom::Width(300);
        height = geom::Height(200);
        pf = mc::PixelFormat::rgba_8888;
    }

    std::shared_ptr<mgg::MockGBMDeviceCreator> mocker;
    std::unique_ptr<mgg::GBMBufferAllocator> allocator;

    // Defaults
    mc::PixelFormat pf;
    geom::Width width;
    geom::Height height;
};

TEST_F(GBMGraphicBufferBasic, dimensions_test)
{
    using namespace testing;

    EXPECT_CALL(*mocker, bo_create(_, _, _, _, _));
    EXPECT_CALL(*mocker, bo_destroy(_));

    std::unique_ptr<mc::Buffer> buffer = allocator->alloc_buffer(width, height, pf);
    ASSERT_EQ(width, buffer->width());
    ASSERT_EQ(height, buffer->height());
}

TEST_F(GBMGraphicBufferBasic, buffer_has_expected_pixel_format)
{
    using namespace testing;

    EXPECT_CALL(*mocker, bo_create(_, _, _, _, _));
    EXPECT_CALL(*mocker, bo_destroy(_));

    std::unique_ptr<mc::Buffer> buffer(allocator->alloc_buffer(width, height, pf));
    ASSERT_EQ(pf, buffer->pixel_format());
}

TEST_F(GBMGraphicBufferBasic, stride_has_sane_value)
{
    using namespace testing;

    EXPECT_CALL(*mocker, bo_create(_, _, _, _, _));
    EXPECT_CALL(*mocker, bo_destroy(_));

    // RGBA 8888 cannot take less than 4 bytes
    // TODO: is there a *maximum* sane value for stride?
    geom::Stride minimum(width.as_uint32_t() * 4);

    std::unique_ptr<mc::Buffer> buffer(allocator->alloc_buffer(width, height, pf));

    ASSERT_LE(minimum, buffer->stride());
}
