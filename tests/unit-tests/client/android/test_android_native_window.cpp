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

#include "mir/graphics/android/mir_native_window.h"
#include "mir/graphics/android/android_driver_interpreter.h"
#include "mir/egl_native_surface.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir/test/doubles/stub_android_native_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

namespace
{

class MockAndroidDriverInterpreter : public mga::AndroidDriverInterpreter
{
public:
    MockAndroidDriverInterpreter()
     : buffer(std::make_shared<mtd::StubAndroidNativeBuffer>())
    {
        using namespace testing;
        ON_CALL(*this, driver_requests_buffer())
            .WillByDefault(Return(buffer.get()));
    }
    MOCK_METHOD0(driver_requests_buffer, mir::graphics::NativeBuffer*());
    MOCK_METHOD2(driver_returns_buffer, void(ANativeWindowBuffer*, int));
    MOCK_METHOD1(dispatch_driver_request_format, void(int));
    MOCK_CONST_METHOD1(driver_requests_info, int(int));
    MOCK_METHOD1(sync_to_display, void(bool));
    MOCK_METHOD1(dispatch_driver_request_buffer_count, void(unsigned int));

    std::shared_ptr<mir::graphics::NativeBuffer> buffer;
};

class AndroidNativeWindowTest : public ::testing::Test
{
protected:
    std::shared_ptr<MockAndroidDriverInterpreter> const mock_driver_interpreter =
        std::make_shared<testing::NiceMock<MockAndroidDriverInterpreter>>();
    mga::MirNativeWindow mir_native_window{mock_driver_interpreter};
    ANativeWindow& window = mir_native_window;
    int const failure_code{-1};
};

}

TEST_F(AndroidNativeWindowTest, native_window_swapinterval)
{
    using namespace testing;

    ASSERT_NE(nullptr, window.setSwapInterval);
    EXPECT_CALL(*mock_driver_interpreter, sync_to_display(true))
        .Times(1);
    window.setSwapInterval(&window, 1);
    Mock::VerifyAndClearExpectations(mock_driver_interpreter.get());

    EXPECT_CALL(*mock_driver_interpreter, sync_to_display(true))
        .Times(1);
    window.setSwapInterval(&window, 2);
    Mock::VerifyAndClearExpectations(mock_driver_interpreter.get());

    EXPECT_CALL(*mock_driver_interpreter, sync_to_display(false))
        .Times(1);
    window.setSwapInterval(&window, 0);
}

/* Query hook tests */
TEST_F(AndroidNativeWindowTest, native_window_query_hook)
{
    using namespace testing;

    int returned_width;
    int const width = 271828;

    ASSERT_NE(nullptr, window.query);
    EXPECT_CALL(*mock_driver_interpreter, driver_requests_info(NATIVE_WINDOW_WIDTH))
        .Times(1)
        .WillOnce(Return(width));

    window.query(&window, NATIVE_WINDOW_WIDTH ,&returned_width);

    EXPECT_EQ(width, returned_width);
}

/* perform hook tests */
TEST_F(AndroidNativeWindowTest, native_window_perform_hook_callable)
{
    int const format = 4;

    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_format(format))
        .Times(1);

    ASSERT_NE(nullptr, window.perform);
    window.perform(&window, NATIVE_WINDOW_SET_BUFFERS_FORMAT, format);
}

TEST_F(AndroidNativeWindowTest, native_window_perform_hook_calls_set_buffer)
{
    int const size = 4;
    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_buffer_count(size));
    ASSERT_NE(nullptr, window.perform);
    window.perform(&window, NATIVE_WINDOW_SET_BUFFER_COUNT, size);
}

/* setSwapInterval hook tests */
TEST_F(AndroidNativeWindowTest, native_window_setswapinterval_hook_callable)
{
    int const swap = 2;

    ASSERT_NE(nullptr, window.setSwapInterval);
    window.setSwapInterval(&window, swap);
}

/* dequeue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_dequeue_hook_callable)
{
    ANativeWindowBuffer* returned_buffer;
    int fence_fd;

    ASSERT_NE(nullptr, window.dequeueBuffer);
    window.dequeueBuffer(&window, &returned_buffer, &fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_returns_right_buffer)
{
    using namespace testing;

    int fake_fd = 4948;
    auto mock_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();
    EXPECT_CALL(*mock_buffer, copy_fence())
        .Times(1)
        .WillOnce(Return(fake_fd));
    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(1)
        .WillOnce(Return(mock_buffer.get()));

    int fence_fd;
    ANativeWindowBuffer* returned_buffer;
    window.dequeueBuffer(&window, &returned_buffer, &fence_fd);

    EXPECT_EQ(mock_buffer->anwb(), returned_buffer);
    EXPECT_EQ(fake_fd, fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_returns_previously_cancelled_buffer)
{
    using namespace testing;
    ANativeWindowBuffer buffer;
    int fence_fd = 33;

    auto rc = window.cancelBuffer(&window, &buffer, fence_fd);
    EXPECT_EQ(0, rc);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(0);

    ANativeWindowBuffer* dequeued_buffer;
    ANativeWindowBuffer* expected_buffer = &buffer;
    window.dequeueBuffer(&window, &dequeued_buffer, &fence_fd);
    EXPECT_EQ(dequeued_buffer, expected_buffer);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_hook_callable)
{
    ANativeWindowBuffer* tmp;

    ASSERT_NE(nullptr, window.dequeueBuffer_DEPRECATED);
    window.dequeueBuffer_DEPRECATED(&window, &tmp);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_returns_right_buffer)
{
    using namespace testing;

    ANativeWindowBuffer* returned_buffer;
    auto mock_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(1)
        .WillOnce(Return(mock_buffer.get()));
    EXPECT_CALL(*mock_buffer, ensure_available_for(mga::BufferAccess::write))
        .Times(1);
    EXPECT_CALL(*mock_buffer, copy_fence())
        .Times(0);

    window.dequeueBuffer_DEPRECATED(&window, &returned_buffer);
    EXPECT_EQ(mock_buffer->anwb(), returned_buffer);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_returns_previously_cancelled_buffer)
{
    using namespace testing;
    ANativeWindowBuffer buffer;

    auto rc = window.cancelBuffer_DEPRECATED(&window, &buffer);
    EXPECT_EQ(0, rc);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(0);

    ANativeWindowBuffer* dequeued_buffer;
    ANativeWindowBuffer* expected_buffer = &buffer;
    window.dequeueBuffer_DEPRECATED(&window, &dequeued_buffer);
    EXPECT_EQ(dequeued_buffer, expected_buffer);
}

/* queue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_queue_hook_callable)
{
    ANativeWindowBuffer* tmp = nullptr;

    ASSERT_NE(nullptr, window.queueBuffer);
    window.queueBuffer(&window, tmp, -1);
}

TEST_F(AndroidNativeWindowTest, native_window_queue_passes_buffer_back)
{
    using namespace testing;
    ANativeWindowBuffer buffer;
    int fence_fd = 33;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, fence_fd))
        .Times(1);

    window.queueBuffer(&window, &buffer, fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_hook_callable)
{
    ANativeWindowBuffer* tmp = nullptr;

    ASSERT_NE(nullptr, window.queueBuffer_DEPRECATED);
    window.queueBuffer_DEPRECATED(&window, tmp);
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_passes_buffer_back)
{
    using namespace testing;
    ANativeWindowBuffer buffer;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer,_))
        .Times(1);

    window.queueBuffer_DEPRECATED(&window, &buffer);
}

/* cancel hook tests */
TEST_F(AndroidNativeWindowTest, native_window_cancel_hooks_callable)
{
    ANativeWindowBuffer* tmp = nullptr;

    ASSERT_NE(nullptr, window.cancelBuffer_DEPRECATED);
    ASSERT_NE(nullptr, window.cancelBuffer);
    window.cancelBuffer_DEPRECATED(&window, tmp);
    window.cancelBuffer(&window, tmp, -1);
}

/* lock hook tests */
TEST_F(AndroidNativeWindowTest, native_window_lock_hook_callable)
{
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, window.lockBuffer_DEPRECATED);
    window.lockBuffer_DEPRECATED(&window, tmp);
}

TEST_F(AndroidNativeWindowTest, native_window_incref_hook_callable)
{
    ASSERT_NE(nullptr, window.common.incRef);
    window.common.incRef(NULL);
}

TEST_F(AndroidNativeWindowTest, native_window_decref_hook_callable)
{
    ASSERT_NE(nullptr, window.common.decRef);
    window.common.decRef(NULL);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_has_proper_rc)
{
    ANativeWindowBuffer* tmp;

    auto ret = window.dequeueBuffer_DEPRECATED(&window, &tmp);
    EXPECT_EQ(0, ret);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_has_proper_rc)
{
    ANativeWindowBuffer* tmp;
    int fencefd;

    auto ret = window.dequeueBuffer(&window, &tmp, &fencefd);
    EXPECT_EQ(0, ret);
}

TEST_F(AndroidNativeWindowTest, native_window_cancel_hook_does_not_call_driver_interpreter)
{
    using namespace testing;
    ANativeWindowBuffer buffer;
    int fence_fd = 33;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, _))
        .Times(0);

    auto rc = window.cancelBuffer(&window, &buffer, fence_fd);
    EXPECT_EQ(0, rc);
}

TEST_F(AndroidNativeWindowTest, returns_error_on_dequeue_buffer_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .WillOnce(Throw(std::runtime_error("")))
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THAT(window.dequeueBuffer(&window, nullptr, nullptr), Eq(failure_code));
    EXPECT_THAT(window.dequeueBuffer_DEPRECATED(&window, nullptr), Eq(failure_code));
}

TEST_F(AndroidNativeWindowTest, returns_error_on_queue_buffer_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(_, _))
        .WillOnce(Throw(std::runtime_error("")))
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THAT(window.queueBuffer(&window, nullptr, 0), Eq(failure_code));
    EXPECT_THAT(window.queueBuffer_DEPRECATED(&window, nullptr), Eq(failure_code));
}

TEST_F(AndroidNativeWindowTest, returns_error_on_query_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_info(_))
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THAT(window.query(&window, 0, nullptr), Eq(failure_code));
}

TEST_F(AndroidNativeWindowTest, returns_error_on_perform_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_format(_))
        .WillOnce(Throw(std::runtime_error("")));

    auto perform = [this] (int key, ...)
    {
        va_list args;
        va_start(args, key);
        EXPECT_THAT(window.perform(&window, key, args), Eq(failure_code));
        va_end(args);
    };
    perform(NATIVE_WINDOW_SET_BUFFERS_FORMAT, 0);
}
