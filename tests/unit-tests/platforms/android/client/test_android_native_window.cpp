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

#include "mir_native_window.h"
#include "android_driver_interpreter.h"
#include "native_window_report.h"
#include "mir/egl_native_surface.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir/test/doubles/stub_android_native_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

namespace
{
struct MockReport : mga::NativeWindowReport
{
    MOCK_CONST_METHOD4(buffer_event, void(mga::BufferEvent, ANativeWindow const*, ANativeWindowBuffer*, int));
    MOCK_CONST_METHOD3(buffer_event, void(mga::BufferEvent, ANativeWindow const*, ANativeWindowBuffer*));
    MOCK_CONST_METHOD3(query_event, void(ANativeWindow const*, int, int));
    MOCK_CONST_METHOD3(perform_event, void(ANativeWindow const*, int, std::vector<int> const&));
};

class MockAndroidDriverInterpreter : public mga::AndroidDriverInterpreter
{
public:
    MockAndroidDriverInterpreter()
     : buffer(std::make_shared<mtd::StubAndroidNativeBuffer>())
    {
        ON_CALL(*this, driver_requests_buffer())
            .WillByDefault(Return(buffer.get()));
    }
    MOCK_METHOD0(driver_requests_buffer, mga::NativeBuffer*());
    MOCK_METHOD2(driver_returns_buffer, void(ANativeWindowBuffer*, int));
    MOCK_METHOD1(dispatch_driver_request_format, void(int));
    MOCK_CONST_METHOD1(driver_requests_info, int(int));
    MOCK_METHOD1(sync_to_display, void(bool));
    MOCK_METHOD1(dispatch_driver_request_buffer_count, void(unsigned int));
    MOCK_METHOD1(dispatch_driver_request_buffer_size, void(mir::geometry::Size));

    std::shared_ptr<mga::NativeBuffer> buffer;
};

class AndroidNativeWindowTest : public ::Test
{
protected:
    std::shared_ptr<MockAndroidDriverInterpreter> const mock_driver_interpreter =
        std::make_shared<NiceMock<MockAndroidDriverInterpreter>>();
    std::shared_ptr<MockReport> const mock_report = std::make_shared<NiceMock<MockReport>>();
    mga::MirNativeWindow mir_native_window{mock_driver_interpreter, mock_report};
    ANativeWindow& window = mir_native_window;
    int const failure_code{-1};
};

}

TEST_F(AndroidNativeWindowTest, use_native_surface_call_replaces_existing_interpreter)
{
    std::shared_ptr<MockAndroidDriverInterpreter> const new_driver_interpreter =
        std::make_shared<NiceMock<MockAndroidDriverInterpreter>>();

    // Test that the call goes to the new interpreter instead of the old one
    EXPECT_CALL(*mock_driver_interpreter, sync_to_display(true))
        .Times(0);

    EXPECT_CALL(*new_driver_interpreter, sync_to_display(true))
        .Times(1);

    mir_native_window.use_native_surface(new_driver_interpreter);
    window.setSwapInterval(&window, 1);
}

TEST_F(AndroidNativeWindowTest, native_window_swapinterval)
{
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
    int returned_width;
    int const width = 271828;
    int key = NATIVE_WINDOW_WIDTH;

    ASSERT_NE(nullptr, window.query);
    EXPECT_CALL(*mock_driver_interpreter, driver_requests_info(NATIVE_WINDOW_WIDTH))
        .Times(1)
        .WillOnce(Return(width));
    EXPECT_CALL(*mock_report, query_event(_, key, width));

    window.query(&window, key, &returned_width);

    EXPECT_EQ(width, returned_width);
}

/* perform hook tests */
TEST_F(AndroidNativeWindowTest, native_window_perform_hook_callable)
{
    int const format = 4;
    int const key = NATIVE_WINDOW_SET_BUFFERS_FORMAT;

    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_format(format))
        .Times(1);

    ASSERT_NE(nullptr, window.perform);
    EXPECT_CALL(*mock_report, perform_event(_, key, _));
    window.perform(&window, key, format);
}

TEST_F(AndroidNativeWindowTest, native_window_perform_hook_calls_set_buffer)
{
    int const size = 4;
    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_buffer_count(size));
    ASSERT_NE(nullptr, window.perform);
    window.perform(&window, NATIVE_WINDOW_SET_BUFFER_COUNT, size);
}

TEST_F(AndroidNativeWindowTest, native_window_perform_hook_calls_set_buffer_size)
{
    mir::geometry::Size size { 1091, 100 };
    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_buffer_size(size));
    ASSERT_NE(nullptr, window.perform);
    window.perform(&window, NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS, size.width, size.height);
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
    EXPECT_CALL(*mock_report, buffer_event(mga::BufferEvent::Dequeue, _, _, _));
    window.dequeueBuffer(&window, &returned_buffer, &fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_returns_right_buffer)
{
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
    ANativeWindowBuffer* returned_buffer;
    auto mock_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(1)
        .WillOnce(Return(mock_buffer.get()));
    EXPECT_CALL(*mock_buffer, ensure_available_for(mga::BufferAccess::write))
        .Times(1);
    EXPECT_CALL(*mock_buffer, copy_fence())
        .Times(0);
    EXPECT_CALL(*mock_report, buffer_event(mga::BufferEvent::Dequeue, _, _));

    window.dequeueBuffer_DEPRECATED(&window, &returned_buffer);
    EXPECT_EQ(mock_buffer->anwb(), returned_buffer);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_returns_previously_cancelled_buffer)
{
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
    ANativeWindowBuffer buffer;
    int fence_fd = 33;

    EXPECT_CALL(*mock_report, buffer_event(mga::BufferEvent::Queue, _, &buffer, fence_fd));
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
    ANativeWindowBuffer buffer;

    EXPECT_CALL(*mock_report, buffer_event(mga::BufferEvent::Queue, _, &buffer));
    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer,_))
        .Times(1);

    window.queueBuffer_DEPRECATED(&window, &buffer);
}

/* cancel hook tests */
TEST_F(AndroidNativeWindowTest, native_window_cancel_hooks_callable)
{
    ANativeWindowBuffer* tmp = nullptr;
    int fence_fd = -1;

    EXPECT_CALL(*mock_report, buffer_event(mga::BufferEvent::Cancel, _, tmp));
    EXPECT_CALL(*mock_report, buffer_event(mga::BufferEvent::Cancel, _, tmp, fence_fd));
    ASSERT_NE(nullptr, window.cancelBuffer_DEPRECATED);
    ASSERT_NE(nullptr, window.cancelBuffer);
    window.cancelBuffer_DEPRECATED(&window, tmp);
    window.cancelBuffer(&window, tmp, fence_fd);
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
    ANativeWindowBuffer buffer;
    int fence_fd = 33;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, _))
        .Times(0);

    auto rc = window.cancelBuffer(&window, &buffer, fence_fd);
    EXPECT_EQ(0, rc);
}

TEST_F(AndroidNativeWindowTest, returns_error_on_dequeue_buffer_failure)
{
    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .WillOnce(Throw(std::runtime_error("")))
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THAT(window.dequeueBuffer(&window, nullptr, nullptr), Eq(failure_code));
    EXPECT_THAT(window.dequeueBuffer_DEPRECATED(&window, nullptr), Eq(failure_code));
}

TEST_F(AndroidNativeWindowTest, returns_error_on_queue_buffer_failure)
{
    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(_, _))
        .WillOnce(Throw(std::runtime_error("")))
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THAT(window.queueBuffer(&window, nullptr, 0), Eq(failure_code));
    EXPECT_THAT(window.queueBuffer_DEPRECATED(&window, nullptr), Eq(failure_code));
}

TEST_F(AndroidNativeWindowTest, returns_error_on_query_failure)
{
    EXPECT_CALL(*mock_driver_interpreter, driver_requests_info(_))
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THAT(window.query(&window, 0, nullptr), Eq(failure_code));
}

TEST_F(AndroidNativeWindowTest, returns_error_on_perform_failure)
{
    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_format(_))
        .WillOnce(Throw(std::runtime_error("")));
    EXPECT_THAT(window.perform(&window, NATIVE_WINDOW_SET_BUFFERS_FORMAT, 0), Eq(failure_code));
}
