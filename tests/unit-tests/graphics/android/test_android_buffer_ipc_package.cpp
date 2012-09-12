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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mga=mir::graphics::android;

class BufferIPCPackageTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        data = {443, 421, 493};
    }

public:
    std::vector<int> data;
    mga::MockBufferHandle mock_buffer_handle;
};

TEST_F(BufferIPCPackageTest, test_int_acquisiton_length)
{
    using namespace testing;

    mga::AndroidBufferIPCPackage package(mock_buffer_handle);
    auto test_vector = package.get_ipc_data();
  
    EXPECT_EQ(data.size(), test_vector.size()); 
}

TEST_F(BufferIPCPackageTest, test_int_ipc_values)
{
    mga::AndroidBufferIPCPackage package(mock_buffer_handle);
    auto test_vector = package.get_ipc_data();

    /* above test tests that they're the same size */
    for(unsigned int i=0; i < test_vector.size(); i++)
    {
        EXPECT_EQ(test_vector[i], data[i]);
    } 
}
