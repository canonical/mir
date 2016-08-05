/*
 * Copyright © 2013 Canonical Ltd.
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

#include "src/platforms/android/server/interpreter_cache.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir/test/doubles/stub_android_native_buffer.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

struct InterpreterResourceTest : public ::testing::Test
{
    void SetUp()
    {
        stub_buffer1 = std::make_shared<mtd::StubBuffer>();
        stub_buffer2 = std::make_shared<mtd::StubBuffer>();
        stub_buffer3 = std::make_shared<mtd::StubBuffer>();
        native_buffer1 = std::make_shared<testing::NiceMock<mtd::MockAndroidNativeBuffer>>();
        native_buffer2 = std::make_shared<mtd::StubAndroidNativeBuffer>();
        native_buffer3 = std::make_shared<mtd::StubAndroidNativeBuffer>();
    }

    std::shared_ptr<mtd::StubBuffer> stub_buffer1;
    std::shared_ptr<mtd::StubBuffer> stub_buffer2;
    std::shared_ptr<mtd::StubBuffer> stub_buffer3;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> native_buffer1;
    std::shared_ptr<mg::NativeBuffer> native_buffer2;
    std::shared_ptr<mg::NativeBuffer> native_buffer3;
};

TEST_F(InterpreterResourceTest, deposit_buffer)
{
    mga::InterpreterCache cache;
    cache.store_buffer(stub_buffer1, native_buffer1);

    auto test_buffer = cache.retrieve_buffer(native_buffer1->anwb());
    EXPECT_EQ(stub_buffer1, test_buffer);
}

TEST_F(InterpreterResourceTest, deposit_many_buffers)
{
    mga::InterpreterCache cache;
    cache.store_buffer(stub_buffer1, native_buffer1);
    cache.store_buffer(stub_buffer2, native_buffer2);
    cache.store_buffer(stub_buffer3, native_buffer3);

    EXPECT_EQ(stub_buffer3, cache.retrieve_buffer(native_buffer3->anwb()));
    EXPECT_EQ(stub_buffer1, cache.retrieve_buffer(native_buffer1->anwb()));
    EXPECT_EQ(stub_buffer2, cache.retrieve_buffer(native_buffer2->anwb()));
}

TEST_F(InterpreterResourceTest, deposit_buffer_has_ownership)
{
    mga::InterpreterCache cache;

    auto native_use_count_before = native_buffer1.use_count();
    auto use_count_before = stub_buffer1.use_count();

    cache.store_buffer(stub_buffer1, native_buffer1);
    EXPECT_EQ(native_use_count_before+1, native_buffer1.use_count());
    EXPECT_EQ(use_count_before+1, stub_buffer1.use_count());

    cache.retrieve_buffer(native_buffer1->anwb());
    EXPECT_EQ(native_use_count_before, native_buffer1.use_count());
    EXPECT_EQ(use_count_before, stub_buffer1.use_count());
}

TEST_F(InterpreterResourceTest, update_usage_for)
{
    int fence_fd = 44;
    mga::InterpreterCache cache;

    EXPECT_CALL(*native_buffer1, update_usage(fence_fd, mga::BufferAccess::write))
        .Times(1);

    cache.store_buffer(stub_buffer1, native_buffer1);
    cache.update_native_fence(native_buffer1->anwb(), fence_fd);

    EXPECT_THROW({
        cache.update_native_fence(nullptr, fence_fd);
    }, std::runtime_error);
}

TEST_F(InterpreterResourceTest, retreive_buffer_with_bad_key_throws)
{
    mga::InterpreterCache cache;
    EXPECT_THROW({
        cache.retrieve_buffer(native_buffer1->anwb());
    }, std::runtime_error);
}
