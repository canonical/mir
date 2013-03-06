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
#include "src/client/client_buffer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace geom=mir::geometry;

namespace
{
struct MockClientBuffer : public mcl::ClientBuffer
{
    MockClientBuffer()
    {
        using namespace testing;
        ON_CALL(*this, get_native_handle())
            .WillByDefault(Return(&buffer));
    }
    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, geom::PixelFormat());

    MOCK_CONST_METHOD0(get_buffer_package, std::shared_ptr<MirBufferPackage>());
    MOCK_METHOD0(get_native_handle, ANativeWindowBuffer*());

    ANativeWindowBuffer buffer;
    native_handle_t handle;
};

struct MockMirSurface : public mcl::ClientSurface
{
    MockMirSurface(MirSurfaceParameters params)
     : params(params)
    {
        using namespace testing;
        ON_CALL(*this, get_parameters())
            .WillByDefault(Return(params));
        ON_CALL(*this, get_current_buffer())
            .WillByDefault(Return(
                std::make_shared<NiceMock<MockClientBuffer>>()));
    }

    MOCK_CONST_METHOD0(get_parameters, MirSurfaceParameters());
    MOCK_METHOD0(get_current_buffer, std::shared_ptr<mcl::ClientBuffer>());
    MOCK_METHOD2(next_buffer, MirWaitHandle*(mir_surface_lifecycle_callback callback, void * context));

    MirSurfaceParameters params;
};


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
        surf_params.width = 530;
        surf_params.height = 715;
        surf_params.pixel_format = mir_pixel_format_abgr_8888;

        mock_surface = std::make_shared<NiceMock<MockMirSurface>>(surf_params);
        mock_client_buffer = std::make_shared<NiceMock<MockClientBuffer>>();
        mock_driver_interpreter = std::make_shared<MockAndroidDriverInterpreter>();
    }

    MirSurfaceParameters surf_params;
    std::shared_ptr<MockMirSurface> mock_surface;
    std::shared_ptr<MockClientBuffer> mock_client_buffer;
    std::shared_ptr<MockAndroidDriverInterpreter> mock_driver_interpreter;
};

/* Query hook tests */
TEST_F(AndroidNativeWindowTest, native_window_query_hook)
{
    using namespace testing;

    int width = 271828;
    mcla::MirNativeWindow anw(mock_driver_interpreter);

    ASSERT_NE(nullptr, anw.query);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_info(NATIVE_WINDOW_WIDTH))
        .Times(1)
        .WillOnce(Return(width));

    int returned_width;
    EXPECT_NO_THROW({
        anw.query(&anw, NATIVE_WINDOW_WIDTH ,&returned_width);
    });
    
    EXPECT_EQ(width, returned_width);
}

/* perform hook tests */
TEST_F(AndroidNativeWindowTest, native_window_perform_hook_callable)
{
    int format = 4;

    mcla::MirNativeWindow anw(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, dispatch_driver_request_format(format))
        .Times(1);

    ASSERT_NE(nullptr, anw.perform);
    EXPECT_NO_THROW({
        anw.perform(&anw, NATIVE_WINDOW_SET_BUFFERS_FORMAT, format);
    });
}

/* setSwapInterval hook tests */
TEST_F(AndroidNativeWindowTest, native_window_setswapinterval_hook_callable)
{
    int swap = 2;
    mcla::MirNativeWindow anw(mock_driver_interpreter);

    ASSERT_NE(nullptr, anw.setSwapInterval);
    EXPECT_NO_THROW({
        anw.setSwapInterval(&anw, swap);
    });
}

/* dequeue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_dequeue_hook_callable)
{
    ANativeWindowBuffer* returned_buffer; 

    mcla::MirNativeWindow anw(mock_driver_interpreter);

    ASSERT_NE(nullptr, anw.dequeueBuffer);
    EXPECT_NO_THROW({
        int fence_fd;
        anw.dequeueBuffer(&anw, &returned_buffer, &fence_fd);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_returns_right_buffer)
{
    using namespace testing;

    ANativeWindowBuffer* returned_buffer; 
    ANativeWindowBuffer fake_buffer;

    mcla::MirNativeWindow anw(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(1)
        .WillOnce(Return(&fake_buffer));

    int fence_fd;
    anw.dequeueBuffer(&anw, &returned_buffer, &fence_fd);

    EXPECT_EQ(&fake_buffer, returned_buffer);
}


TEST_F(AndroidNativeWindowTest, native_window_dequeue_indicates_buffer_immediately_usable)
{
    ANativeWindowBuffer* returned_buffer; 
    mcla::MirNativeWindow anw(mock_driver_interpreter);

    int fence_fd;
    anw.dequeueBuffer(&anw, &returned_buffer, &fence_fd);
    EXPECT_EQ(-1, fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp;

    ASSERT_NE(nullptr, anw.dequeueBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        anw.dequeueBuffer_DEPRECATED(&anw, &tmp);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_returns_right_buffer)
{
    using namespace testing;

    ANativeWindowBuffer* returned_buffer; 
    ANativeWindowBuffer fake_buffer;

    mcla::MirNativeWindow anw(mock_driver_interpreter);

    EXPECT_CALL(*mock_driver_interpreter, driver_requests_buffer())
        .Times(1)
        .WillOnce(Return(&fake_buffer));

    anw.dequeueBuffer_DEPRECATED(&anw, &returned_buffer);
    EXPECT_EQ(&fake_buffer, returned_buffer);
}

/* queue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_queue_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, anw.queueBuffer);
    EXPECT_NO_THROW({
        anw.queueBuffer(&anw, tmp, -1);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_queue_passes_buffer_and_fence_back)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer buffer;
    int fence_fd = 33;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, fence_fd))
        .Times(1);

    anw.queueBuffer(&anw, &buffer, fence_fd);
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, anw.queueBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        anw.queueBuffer_DEPRECATED(&anw, tmp);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_always_indicates_no_wait)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer buffer;

    EXPECT_CALL(*mock_driver_interpreter, driver_returns_buffer(&buffer, -1))
        .Times(1);

    anw.queueBuffer_DEPRECATED(&anw, &buffer);
}

/* cancel hook tests */
TEST_F(AndroidNativeWindowTest, native_window_cancel_hooks_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, anw.cancelBuffer_DEPRECATED);
    ASSERT_NE(nullptr, anw.cancelBuffer);
    EXPECT_NO_THROW({
        anw.cancelBuffer_DEPRECATED(&anw, tmp);
        anw.cancelBuffer(&anw, tmp, -1);
    });
}

/* lock hook tests */
TEST_F(AndroidNativeWindowTest, native_window_lock_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, anw.lockBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        anw.lockBuffer_DEPRECATED(&anw, tmp);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_incref_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);

    ASSERT_NE(nullptr, anw.common.incRef);
    EXPECT_NO_THROW({
        anw.common.incRef(NULL);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_decref_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);

    ASSERT_NE(nullptr, anw.common.decRef);
    EXPECT_NO_THROW({
        anw.common.decRef(NULL);
    });
}
