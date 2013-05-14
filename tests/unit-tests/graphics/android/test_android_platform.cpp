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

#include "mir/graphics/null_display_report.h"
#include "src/server/graphics/android/android_platform.h"
#include "mir_test_doubles/mock_buffer.h"
#include <system/window.h>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

class PlatformBufferIPCPackaging : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        stub_display_report = std::make_shared<mg::NullDisplayReport>();
        stride = geom::Stride(300*4);

        num_ints = 43;
        num_fds = 55;
        auto handle_size = sizeof(native_handle_t) + (sizeof(int)*(num_ints + num_fds));
        auto native_buffer_raw = (native_handle_t*) ::operator new(handle_size);
        native_buffer_handle = std::shared_ptr<native_handle_t>(native_buffer_raw);

        native_buffer_handle->numInts = num_ints;
        native_buffer_handle->numFds = num_fds;
        for(auto i=0u; i< (num_ints+num_fds); i++)
        {
            native_buffer_handle->data[i] = i;
        }
 
        anwb = std::make_shared<ANativeWindowBuffer>();
        anwb->stride = (int) stride.as_uint32_t();
        anwb->handle  = native_buffer_handle.get();


        mock_buffer = std::make_shared<mtd::MockBuffer>();
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(testing::Return(anwb));
        ON_CALL(*mock_buffer, stride())
            .WillByDefault(testing::Return(stride));
    }

    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<ANativeWindowBuffer> anwb;
    std::shared_ptr<native_handle_t> native_buffer_handle;
    std::shared_ptr<mg::DisplayReport> stub_display_report;
    geom::Stride stride;
    unsigned int num_ints, num_fds;
};

/* ipc packaging tests */
TEST_F(PlatformBufferIPCPackaging, test_ipc_data_packed_correctly)
{
    mga::AndroidPlatform platform(stub_display_report);

    auto package = platform.create_buffer_ipc_package(mock_buffer);

    ASSERT_EQ(num_ints, package->ipc_data.size());
    ASSERT_EQ(num_fds, package->ipc_fds.size());

    int offset = 0;
    for(auto it=package->ipc_fds.begin(); it != package->ipc_fds.end(); it++)
    {
        EXPECT_EQ(anwb->handle->data[offset++], *it);
    }

    for(auto it= package->ipc_data.begin(); it != package->ipc_data.end(); it++)
    {
        EXPECT_EQ(anwb->handle->data[offset++], *it);
    }
}
