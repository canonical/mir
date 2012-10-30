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

#include "src/graphics/android/android_buffer.h"
#include "mir/compositor/buffer_ipc_package.h"
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
        using namespace testing;
        mock_buffer_handle = std::make_shared<mga::MockBufferHandle>();
        mock_alloc_device = std::make_shared<mga::MockAllocAdaptor>(mock_buffer_handle);

        /* set up common defaults */
        pf = geom::PixelFormat::rgba_8888;
        size = geom::Size{geom::Width{300}, geom::Height{200}};

        EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _))
            .Times(AtLeast(0));
    }

    std::shared_ptr<mga::MockAllocAdaptor> mock_alloc_device;
    std::shared_ptr<mga::MockBufferHandle> mock_buffer_handle;
    geom::PixelFormat pf;
    geom::Size size;
};


TEST_F(AndroidGraphicBufferBasic, basic_allocation_is_non_null)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _));

    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, size, pf));

    EXPECT_NE((int)buffer.get(), NULL);
}

TEST_F(AndroidGraphicBufferBasic, usage_type_is_set_to_hardware_by_default)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, mga::BufferUsage::use_hardware));

    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, size, pf));
}

TEST_F(AndroidGraphicBufferBasic, size_query_test)
{
    using namespace testing;

    geom::Size test_size{geom::Width{443}, geom::Height{667}};

    EXPECT_CALL(*mock_buffer_handle, size())
    .Times(Exactly(1))
    .WillOnce(Return(test_size));
    EXPECT_CALL(*mock_alloc_device, alloc_buffer( size, _, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, size, pf));

    EXPECT_EQ(test_size, buffer->size());
}

TEST_F(AndroidGraphicBufferBasic, format_passthrough_test)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, pf, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, size, pf));

}

TEST_F(AndroidGraphicBufferBasic, format_queries_handle_test)
{
    using namespace testing;

    geom::PixelFormat pf2 = geom::PixelFormat::rgb_888;

    EXPECT_CALL(*mock_buffer_handle, format())
    .Times(Exactly(1))
    .WillOnce(Return(pf2));
    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _ , _, _ ));
    std::shared_ptr<mc::Buffer> buffer(new mga::AndroidBuffer(mock_alloc_device, size, pf));

    EXPECT_EQ(buffer->pixel_format(), pf2);

}

TEST_F(AndroidGraphicBufferBasic, queries_native_window_for_ipc_ptr)
{
    using namespace testing;

    auto dummy_ipc_package = std::make_shared<mc::BufferIPCPackage>();
 
    EXPECT_CALL(*mock_buffer_handle, get_ipc_package())
        .Times(Exactly(1))
        .WillOnce(Return(dummy_ipc_package));

    auto buffer = std::make_shared<mga::AndroidBuffer>(mock_alloc_device, size, pf);
    auto returned_package = buffer->get_ipc_package();

    EXPECT_EQ(returned_package, dummy_ipc_package);
}

