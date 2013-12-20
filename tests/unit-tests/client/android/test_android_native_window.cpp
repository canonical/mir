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
#include "src/client/mir_client_surface.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test_doubles/mock_fence.h"

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

    std::shared_ptr<mir::graphics::NativeBuffer> buffer;
};
}

class AndroidNativeWindowTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        mock_driver_interpreter = std::make_shared<NiceMock<MockAndroidDriverInterpreter>>();
    }

    std::shared_ptr<MockAndroidDriverInterpreter> mock_driver_interpreter;
};

TEST_F(AndroidNativeWindowTest, native_window_swapinterval)
{
    using namespace testing;

    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->setSwapInterval);
    EXPECT_CALL(*mock_driver_interpreter, sync_to_display(true))
        .Times(1);
    window->setSwapInterval(window.get(), 1);
    Mock::VerifyAndClearExpectations(window.get());

    EXPECT_CALL(*mock_driver_interpreter, sync_to_display(true))
        .Times(1);
    window->setSwapInterval(window.get(), 2);
    Mock::VerifyAndClearExpectations(window.get());

    EXPECT_CALL(*mock_driver_interpreter, sync_to_display(false))
        .Times(1);
    window->setSwapInterval(window.get(), 0);
}

/* Query hook tests */
TEST_F(AndroidNativeWindowTest, native_window_query_hook)
{
    using namespace testing;

    int returned_width, width = 271828;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->query);
    EXPECT_CALL(*mock_driver_interpreter, driver_requests_info(NATIVE_WINDOW_WIDTH))
        .Times(1)
        .WillOnce(Return(width));

    window->query(window.get(), NATIVE_WINDOW_WIDTH ,&returned_width);

    EXPECT_EQ(width, returned_width);
}

/* perform hook tests */
TEST_F(AndroidNativeWindowTest, native_window_perform_hook_callable)
{
    int format = 4;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_format(format))
        .Times(1);

    ASSERT_NE(nullptr, window->perform);
    window->perform(window.get(), NATIVE_WINDOW_SET_BUFFERS_FORMAT, format);
}

/* setSwapInterval hook tests */
TEST_F(AndroidNativeWindowTest, native_window_setswapinterval_hook_callable)
{
    int swap = 2;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->setSwapInterval);
    window->setSwapInterval(window.get(), swap);
}

/* dequeue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_dequeue_hook_callable)
{
    ANativeWindowBuffer* returned_buffer;
    int fence_fd;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->dequeueBuffer);
    window->dequeueBuffer(window.get(), &returned_buffer, &fence_fd);
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

    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    int fence_fd;
    ANativeWindowBuffer* returned_buffer;
    window->dequeueBuffer(window.get(), &returned_buffer, &fence_fd);

    EXPECT_EQ(mock_buffer->anwb(), returned_buffer);
    EXPECT_EQ(fake_fd, fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_hook_callable)
{
    ANativeWindowBuffer* tmp;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->dequeueBuffer_DEPRECATED);
    window->dequeueBuffer_DEPRECATED(window.get(), &tmp);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_returns_right_buffer)
{
    using namespace testing;

    ANativeWindowBuffer* returned_buffer;
    auto mock_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(1)
        .WillOnce(Return(mock_buffer.get()));
    EXPECT_CALL(*mock_buffer, wait_for_content())
        .Times(1);
    EXPECT_CALL(*mock_buffer, copy_fence())
        .Times(0);

    window->dequeueBuffer_DEPRECATED(window.get(), &returned_buffer);
    EXPECT_EQ(mock_buffer->anwb(), returned_buffer);
}

/* queue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_queue_hook_callable)
{
    ANativeWindowBuffer* tmp = nullptr;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->queueBuffer);
    window->queueBuffer(window.get(), tmp, -1);
}

TEST_F(AndroidNativeWindowTest, native_window_queue_passes_buffer_back)
{
    using namespace testing;
    ANativeWindowBuffer buffer;
    int fence_fd = 33;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, fence_fd))
        .Times(1);

    window->queueBuffer(window.get(), &buffer, fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_hook_callable)
{
    ANativeWindowBuffer* tmp = nullptr;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->queueBuffer_DEPRECATED);
    window->queueBuffer_DEPRECATED(window.get(), tmp);
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_passes_buffer_back)
{
    using namespace testing;
    ANativeWindowBuffer buffer;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer,_))
        .Times(1);

    window->queueBuffer_DEPRECATED(window.get(), &buffer);
}

/* cancel hook tests */
TEST_F(AndroidNativeWindowTest, native_window_cancel_hooks_callable)
{
    ANativeWindowBuffer* tmp = nullptr;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->cancelBuffer_DEPRECATED);
    ASSERT_NE(nullptr, window->cancelBuffer);
    window->cancelBuffer_DEPRECATED(window.get(), tmp);
    window->cancelBuffer(window.get(), tmp, -1);
}

/* lock hook tests */
TEST_F(AndroidNativeWindowTest, native_window_lock_hook_callable)
{
    ANativeWindowBuffer* tmp = 0x0;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->lockBuffer_DEPRECATED);
    window->lockBuffer_DEPRECATED(window.get(), tmp);
}

TEST_F(AndroidNativeWindowTest, native_window_incref_hook_callable)
{
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->common.incRef);
    window->common.incRef(NULL);
}

TEST_F(AndroidNativeWindowTest, native_window_decref_hook_callable)
{
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, window->common.decRef);
    window->common.decRef(NULL);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_has_proper_rc)
{
    ANativeWindowBuffer* tmp;
    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    auto ret = window->dequeueBuffer_DEPRECATED(window.get(), &tmp);
    EXPECT_EQ(0, ret);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_has_proper_rc)
{
    ANativeWindowBuffer* tmp;
    int fencefd;

    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);

    auto ret = window->dequeueBuffer(window.get(), &tmp, &fencefd);
    EXPECT_EQ(0, ret);
}

TEST_F(AndroidNativeWindowTest, native_window_cancel_hook_behavior)
{
    using namespace testing;
    ANativeWindowBuffer buffer;
    int fence_fd = 33;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, _))
        .Times(1);

    std::shared_ptr<ANativeWindow> window = std::make_shared<mga::MirNativeWindow>(mock_driver_interpreter);
    auto rc = window->cancelBuffer(window.get(), &buffer, fence_fd);
    EXPECT_EQ(0, rc);
}
