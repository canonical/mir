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

#include "src/client/mir_client_surface.h"
#include "src/client/android/mir_native_window.h"
#include "src/client/android/android_driver_interpreter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
namespace mcla=mir::client::android;

namespace
{
class MockAndroidDriverInterpreter : public mcla::AndroidDriverInterpreter
{
public:
    MOCK_METHOD0(driver_requests_buffer, ANativeWindowBuffer*());
    MOCK_METHOD2(driver_returns_buffer, void(ANativeWindowBuffer*, int));
    MOCK_METHOD1(dispatch_driver_request_format, void(int));
    MOCK_CONST_METHOD1(driver_requests_info, int(int));
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

/* Query hook tests */
TEST_F(AndroidNativeWindowTest, native_window_query_hook)
{
    using namespace testing;

    int width = 271828;
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, win->query);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_info(NATIVE_WINDOW_WIDTH))
        .Times(1)
        .WillOnce(Return(width));

    int returned_width;
    EXPECT_NO_THROW({
        win->query(win.get(), NATIVE_WINDOW_WIDTH ,&returned_width);
    });
    
    EXPECT_EQ(width, returned_width);
}

/* perform hook tests */
TEST_F(AndroidNativeWindowTest, native_window_perform_hook_callable)
{
    int format = 4;

    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_format(format))
        .Times(1);

    ASSERT_NE(nullptr, win->perform);
    EXPECT_NO_THROW({
        win->perform(win.get(), NATIVE_WINDOW_SET_BUFFERS_FORMAT, format);
    });
}

/* setSwapInterval hook tests */
TEST_F(AndroidNativeWindowTest, native_window_setswapinterval_hook_callable)
{
    int swap = 2;
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, win->setSwapInterval);
    EXPECT_NO_THROW({
        win->setSwapInterval(win.get(), swap);
    });
}

/* dequeue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_dequeue_hook_callable)
{
    ANativeWindowBuffer* returned_buffer; 

    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, win->dequeueBuffer);
    EXPECT_NO_THROW({
        int fence_fd;
        win->dequeueBuffer(win.get(), &returned_buffer, &fence_fd);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_returns_right_buffer)
{
    using namespace testing;

    ANativeWindowBuffer* returned_buffer; 
    ANativeWindowBuffer fake_buffer;

    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(1)
        .WillOnce(Return(&fake_buffer));

    int fence_fd;
    win->dequeueBuffer(win.get(), &returned_buffer, &fence_fd);

    EXPECT_EQ(&fake_buffer, returned_buffer);
}


TEST_F(AndroidNativeWindowTest, native_window_dequeue_indicates_buffer_immediately_usable)
{
    ANativeWindowBuffer* returned_buffer; 
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    int fence_fd;
    win->dequeueBuffer(win.get(), &returned_buffer, &fence_fd);
    EXPECT_EQ(-1, fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_hook_callable)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer* tmp;

    ASSERT_NE(nullptr, win->dequeueBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        win->dequeueBuffer_DEPRECATED(win.get(), &tmp);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_returns_right_buffer)
{
    using namespace testing;

    ANativeWindowBuffer* returned_buffer; 
    ANativeWindowBuffer fake_buffer;

    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(1)
        .WillOnce(Return(&fake_buffer));

    win->dequeueBuffer_DEPRECATED(win.get(), &returned_buffer);
    EXPECT_EQ(&fake_buffer, returned_buffer);
}

/* queue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_queue_hook_callable)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, win->queueBuffer);
    EXPECT_NO_THROW({
        win->queueBuffer(win.get(), tmp, -1);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_queue_passes_buffer_and_fence_back)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer buffer;
    int fence_fd = 33;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, fence_fd))
        .Times(1);

    win->queueBuffer(win.get(), &buffer, fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_hook_callable)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, win->queueBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        win->queueBuffer_DEPRECATED(win.get(), tmp);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_always_indicates_no_wait)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer buffer;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, -1))
        .Times(1);

    win->queueBuffer_DEPRECATED(win.get(), &buffer);
}

/* cancel hook tests */
TEST_F(AndroidNativeWindowTest, native_window_cancel_hooks_callable)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, win->cancelBuffer_DEPRECATED);
    ASSERT_NE(nullptr, win->cancelBuffer);
    EXPECT_NO_THROW({
        win->cancelBuffer_DEPRECATED(win.get(), tmp);
        win->cancelBuffer(win.get(), tmp, -1);
    });
}

/* lock hook tests */
TEST_F(AndroidNativeWindowTest, native_window_lock_hook_callable)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, win->lockBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        win->lockBuffer_DEPRECATED(win.get(), tmp);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_incref_hook_callable)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, win->common.incRef);
    EXPECT_NO_THROW({
        win->common.incRef(NULL);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_decref_hook_callable)
{
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ASSERT_NE(nullptr, win->common.decRef);
    EXPECT_NO_THROW({
        win->common.decRef(NULL);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_has_proper_rc)
{
    using namespace testing;
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer* tmp;

    auto ret = win->dequeueBuffer_DEPRECATED(win.get(), &tmp);
    EXPECT_EQ(0, ret);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_has_proper_rc)
{
    using namespace testing;
    int fencefd;
    std::shared_ptr<ANativeWindow> win = std::make_shared<mcla::MirNativeWindow>(mock_driver_interpreter);

    ANativeWindowBuffer* tmp;

    auto ret = win->dequeueBuffer(win.get(), &tmp, &fencefd);
    EXPECT_EQ(0, ret);
}

