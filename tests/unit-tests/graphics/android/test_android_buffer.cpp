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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/android_buffer.h"
#include "mir_test/mock_alloc_adaptor.h"

#include <hardware/gralloc.h> 
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

class AndroidGraphicBufferBasic : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_alloc_device = std::shared_ptr<mg::MockAllocAdaptor> (new mg::MockAllocAdaptor);

        /* set up common defaults */
        pf = mc::PixelFormat::rgba_8888;
        width = geom::Width(300);
        height = geom::Height(200);

    }

    native_handle_t native_handle;
    std::shared_ptr<mg::MockAllocAdaptor> mock_alloc_device;
    mc::PixelFormat pf;
    geom::Width width;
    geom::Height height;
};


TEST_F(AndroidGraphicBufferBasic, basic_allocation_is_non_null)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _, _, _, _));

    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, width, height, pf));

    EXPECT_NE((int)buffer.get(), NULL);
}

TEST_F(AndroidGraphicBufferBasic, usage_type_is_set_to_hardware_by_default)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _, _, _, mg::BufferUsage::use_hardware));

    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, width, height, pf));
}

TEST_F(AndroidGraphicBufferBasic, dimensions_test)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, width, height, _, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, width, height, pf));

    EXPECT_EQ(width, buffer->width());
    EXPECT_EQ(height, buffer->height());
}

TEST_F(AndroidGraphicBufferBasic, format_passthrough_test)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _, _, pf, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, width, height, pf));

}

TEST_F(AndroidGraphicBufferBasic, format_echo_test)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _, _ , _, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, width, height, pf));

    EXPECT_EQ(buffer->pixel_format(), pf);

}

