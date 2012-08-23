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
#include "mir/graphics/graphic_alloc_adaptor.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace geom=mir::geometry;

namespace mir
{
namespace graphics
{

class MockAllocAdaptor : public GraphicAllocAdaptor
{
public:
    MockAllocAdaptor()
    {
        using namespace testing;

        ON_CALL(*this, alloc_buffer(_,_,_,_,_,_))
        .WillByDefault(Return(true));
    }

    MOCK_METHOD6(alloc_buffer, bool(std::shared_ptr<BufferHandle>&, geometry::Stride&, geometry::Width, geometry::Height, compositor::PixelFormat, BufferUsage));
};
}
}

class GBMGraphicBufferBasic : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_alloc_device = std::shared_ptr<mg::MockAllocAdaptor> (new mg::MockAllocAdaptor);

        pf = mc::PixelFormat::rgba_8888;
        width = geom::Width(300);
        height = geom::Height(200);
    }

    std::shared_ptr<mg::MockAllocAdaptor> mock_alloc_device;
    mc::PixelFormat pf;
    geom::Width width;
    geom::Height height;
};

TEST_F(GBMGraphicBufferBasic, basic_allocation_is_non_null)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _, _, _, _));

    std::shared_ptr<mc::Buffer> buffer(new mgg::GBMBuffer(mock_alloc_device, width, height, pf));

    EXPECT_NE(reinterpret_cast<intptr_t>(buffer.get()), NULL);
}

TEST_F(GBMGraphicBufferBasic, usage_type_is_set_to_hardware_by_default)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _, _, _, mg::BufferUsage::use_hardware));

    std::shared_ptr<mc::Buffer> buffer(new mgg::GBMBuffer(mock_alloc_device, width, height, pf));
}

TEST_F(GBMGraphicBufferBasic, dimensions_test)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, width, height, _, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mgg::GBMBuffer(mock_alloc_device, width, height, pf));

    EXPECT_EQ(width, buffer->width());
    EXPECT_EQ(height, buffer->height());
}

TEST_F(GBMGraphicBufferBasic, format_passthrough_test)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _, _, pf, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mgg::GBMBuffer(mock_alloc_device, width, height, pf));

}

TEST_F(GBMGraphicBufferBasic, format_echo_test)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _, _ , _, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mgg::GBMBuffer(mock_alloc_device, width, height, pf));

    EXPECT_EQ(buffer->pixel_format(), pf);

}
