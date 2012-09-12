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

#include "mir/graphics/android/android_buffer_ipc_package.h"
#include "mir_test/mock_alloc_adaptor.h"

#include <system/window.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mga=mir::graphics::android;

class BufferIPCPackageTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        int num_ints = 41, num_fds = 11;
        int total = num_ints + num_fds;

        /* c tricks for android header */
        int * data_handle = (int*) malloc( 3 * sizeof(int) + /* version, numFds, numInts */
                                           total * sizeof(int));
        native_handle = (native_handle_t*) data_handle;

        /* fill header */ 
        native_handle->version = 0xbead;
        native_handle->numInts = num_ints;
        native_handle->numFds = num_fds; 
        for (int i=0; i<total; i++)
        {
            native_handle->data[i] = i*3;
        } 
        
        ON_CALL(mock_buffer_handle, get_egl_client_buffer())
            .WillByDefault(Return((EGLClientBuffer) native_handle)); 
    }

    virtual void TearDown()
    {
        delete native_handle;
    }

public:

    mga::MockBufferHandle mock_buffer_handle;

    native_handle_t* native_handle;
};

TEST_F(BufferIPCPackageTest, test_data_packed_with_correct_size)
{
    using namespace testing;

    mga::AndroidBufferIPCPackage package(&mock_buffer_handle);
    auto test_vector = package.get_ipc_data();
  
    EXPECT_EQ(native_handle->numInts, (int) test_vector.size()); 
}

TEST_F(BufferIPCPackageTest, test_data_packed_with_correct_data)
{
    using namespace testing;

    mga::AndroidBufferIPCPackage package(&mock_buffer_handle);
    auto test_vector = package.get_ipc_data();

    int fd_offset = native_handle->numFds; 
    for(auto it= test_vector.begin(); it != test_vector.end(); it++)
    { 
        EXPECT_EQ(native_handle->data[fd_offset++], *it);
    } 
}
