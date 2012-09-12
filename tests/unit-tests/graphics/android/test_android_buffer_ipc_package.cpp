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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc=mir::compositor;

class BufferIPCPackageTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        std::vector<int> data = {443, 421, 493};
    }

public:
    std::vector<int> data;
};

TEST_F(BufferIPCPackageTest, test_int_acquisiton_length)
{
    using namespace testing;

    mc::AndroidBufferIPCPackage package(data);
    auto test_vector = package.get_ipc_data();
   
    EXPECT_EQ(data.length(), test_vector.length()); 
}

