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

#include "src/server/graphics/android/android_buffer.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir_test_doubles/mock_alloc_adaptor.h"

#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

class AndroidGraphicBufferBasic : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        mock_buffer_handle = std::make_shared<NiceMock<mtd::MockBufferHandle>>();
        mock_alloc_device = std::make_shared<NiceMock<mtd::MockAllocAdaptor>>(mock_buffer_handle);

        pf = geom::PixelFormat::abgr_8888;
        size = geom::Size{geom::Width{300}, geom::Height{200}};
    }

    std::shared_ptr<mtd::MockAllocAdaptor> mock_alloc_device;
    std::shared_ptr<mtd::MockBufferHandle> mock_buffer_handle;
    geom::PixelFormat pf;
    geom::Size size;
};


TEST_F(AndroidGraphicBufferBasic, basic_allocation_uses_alloc_device)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, _));

    mga::AndroidBuffer buffer(mock_alloc_device, size, pf);
}

TEST_F(AndroidGraphicBufferBasic, usage_type_is_set_to_hardware_by_default)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, _, mga::BufferUsage::use_hardware));

    mga::AndroidBuffer buffer(mock_alloc_device, size, pf);
}

TEST_F(AndroidGraphicBufferBasic, size_query_test)
{
    using namespace testing;

    geom::Size expected_size{geom::Width{443}, geom::Height{667}};

    EXPECT_CALL(*mock_buffer_handle, size())
    .Times(Exactly(1))
    .WillOnce(Return(expected_size));
    EXPECT_CALL(*mock_alloc_device, alloc_buffer( size, _, _ ));
    mga::AndroidBuffer buffer(mock_alloc_device, size, pf);

    EXPECT_EQ(expected_size, buffer.size());
}

TEST_F(AndroidGraphicBufferBasic, format_passthrough_test)
{
    using namespace testing;

    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _, pf, _ ));
    mga::AndroidBuffer buffer(mock_alloc_device, size, pf);
}

TEST_F(AndroidGraphicBufferBasic, format_queries_handle_test)
{
    using namespace testing;

    geom::PixelFormat expected_pf = geom::PixelFormat::bgr_888;

    EXPECT_CALL(*mock_buffer_handle, format())
    .Times(Exactly(1))
    .WillOnce(Return(expected_pf));
    EXPECT_CALL(*mock_alloc_device, alloc_buffer( _ , _, _ ));

    mga::AndroidBuffer buffer(mock_alloc_device, size, pf);

    EXPECT_EQ(expected_pf, buffer.pixel_format());
}

TEST_F(AndroidGraphicBufferBasic, queries_native_window_for_ipc_ptr)
{
    using namespace testing;

    auto expected_ipc_package = std::make_shared<mc::BufferIPCPackage>();

    EXPECT_CALL(*mock_buffer_handle, get_ipc_package())
        .Times(Exactly(1))
        .WillOnce(Return(expected_ipc_package));

    mga::AndroidBuffer buffer(mock_alloc_device, size, pf);

    EXPECT_EQ(expected_ipc_package, buffer.get_ipc_package());
}

TEST_F(AndroidGraphicBufferBasic, queries_native_window_for_stride)
{
    using namespace testing;

    geom::Stride expected_stride{43};
    EXPECT_CALL(*mock_buffer_handle, stride())
        .Times(Exactly(1))
        .WillOnce(Return(expected_stride));

    mga::AndroidBuffer buffer(mock_alloc_device, size, pf);

    EXPECT_EQ(expected_stride, buffer.stride());
}
