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

#include "src/server/graphics/android/interpreter_cache.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

struct InterpreterResourceTest : public ::testing::Test
{
    void SetUp()
    {
        stub_buffer1 = std::make_shared<mtd::StubBuffer>();
        stub_buffer2 = std::make_shared<mtd::StubBuffer>();
        stub_buffer3 = std::make_shared<mtd::StubBuffer>();
        key1 = (ANativeWindowBuffer*)0x1;
        key2 = (ANativeWindowBuffer*)0x2;
        key3 = (ANativeWindowBuffer*)0x3;
    }

    std::shared_ptr<mtd::StubBuffer> stub_buffer1;
    std::shared_ptr<mtd::StubBuffer> stub_buffer2;
    std::shared_ptr<mtd::StubBuffer> stub_buffer3;
    ANativeWindowBuffer *key1;
    ANativeWindowBuffer *key2;
    ANativeWindowBuffer *key3; 
};

TEST_F(InterpreterResourceTest, deposit_buffer)
{
    mga::InterpreterCache cache;
    cache.store_buffer(stub_buffer1, key1);

    auto test_buffer = cache.retrieve_buffer(key1);
    EXPECT_EQ(stub_buffer1, test_buffer);
}

TEST_F(InterpreterResourceTest, deposit_many_buffers)
{
    mga::InterpreterCache cache;
    cache.store_buffer(stub_buffer1, key1);
    cache.store_buffer(stub_buffer2, key2);
    cache.store_buffer(stub_buffer3, key3);

    EXPECT_EQ(stub_buffer3, cache.retrieve_buffer(key3));
    EXPECT_EQ(stub_buffer1, cache.retrieve_buffer(key1));
    EXPECT_EQ(stub_buffer2, cache.retrieve_buffer(key2));
}

TEST_F(InterpreterResourceTest, deposit_buffer_has_ownership)
{
    mga::InterpreterCache cache;

    auto use_count_before = stub_buffer1.use_count();
    cache.store_buffer(stub_buffer1, key1);
    EXPECT_EQ(use_count_before+1, stub_buffer1.use_count());
    cache.retrieve_buffer(key1);
    EXPECT_EQ(use_count_before, stub_buffer1.use_count());
}

TEST_F(InterpreterResourceTest, retreive_buffer_with_bad_key_throws)
{
    mga::InterpreterCache cache;
    EXPECT_THROW({
        cache.retrieve_buffer(key1);
    }, std::runtime_error);
}

#if 0
//doesnt make sense
TEST_F(InternalClientWindow, driver_requests_buffer_ownership)
{
    using namespace testing;
    mga::InternalClientWindow interpreter(std::move(mock_swapper), mock_cache);

    auto use_count_before = mock_buffer.use_count();
    auto test_anwb = interpreter.driver_requests_buffer();
    EXPECT_EQ(use_count_before + 1, mock_buffer.use_count());

    std::shared_ptr<mga::SyncObject> fake_sync;
    interpreter.driver_returns_buffer(test_anwb, fake_sync);
    EXPECT_EQ(use_count_before, mock_buffer.use_count());
}

TEST_F(ServerRenderWindowTest, throw_if_driver_returns_weird_buffer)
{
    using namespace testing;
    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    auto stub_anw = std::make_shared<ANativeWindowBuffer>();

    EXPECT_CALL(*mock_swapper, compositor_release(_))
        .Times(0);

    EXPECT_THROW({
        render_window.driver_returns_buffer(nullptr, stub_sync);
    }, std::runtime_error); 
}

/* note: in real usage, sync is enforced by the swapper class. we make use of the mock's non-blocking 
         to do the tests. */
TEST_F(ServerRenderWindowTest, driver_is_done_with_a_few_buffers_properly)
{
    using namespace testing;
    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    auto stub_anw = std::make_shared<ANativeWindowBuffer>();
    auto stub_anw2 = std::make_shared<ANativeWindowBuffer>();
    auto stub_anw3 = std::make_shared<ANativeWindowBuffer>();

    EXPECT_CALL(*mock_swapper, compositor_acquire())
        .Times(3)
        .WillOnce(Return(mock_buffer1))
        .WillOnce(Return(mock_buffer2))
        .WillOnce(Return(mock_buffer3));
    EXPECT_CALL(*mock_buffer1, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw));
    EXPECT_CALL(*mock_buffer2, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw2));
    EXPECT_CALL(*mock_buffer3, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw3));

    auto handle1 = render_window.driver_requests_buffer();
    auto handle2 = render_window.driver_requests_buffer();
    auto handle3 = render_window.driver_requests_buffer();
    testing::Mock::VerifyAndClearExpectations(mock_swapper.get());

    std::shared_ptr<mc::Buffer> buf1 = mock_buffer1;
    std::shared_ptr<mc::Buffer> buf2 = mock_buffer2;
    std::shared_ptr<mc::Buffer> buf3 = mock_buffer3;
    EXPECT_CALL(*mock_swapper, compositor_release(buf1))
        .Times(1);
    EXPECT_CALL(*mock_swapper, compositor_release(buf2))
        .Times(1);
    EXPECT_CALL(*mock_swapper, compositor_release(buf3))
        .Times(1);

    render_window.driver_returns_buffer(handle1, stub_sync);
    render_window.driver_returns_buffer(handle2, stub_sync);
    render_window.driver_returns_buffer(handle3, stub_sync);
    testing::Mock::VerifyAndClearExpectations(mock_swapper.get());
}

#endif
