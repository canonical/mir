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

class AndroidDriverInterpreter
{
    virtual void driver_requests_buffer() = 0;
    virtual void driver_returns_buffer() = 0;
    virtual void dispatch_driver_request_format() = 0;
    virtual void dispatch_driver_request_swapinterval() = 0;
    virtual void driver_requests_info() = 0;
};

class MockAndroidDriverInterpreter : AndroidDriverInterpreter
{
    MOCK_METHOD0(driver_requests_buffer, void());
    MOCK_METHOD0(driver_returns_buffer, void());
    MOCK_METHOD0(dispatch_driver_request_format, void());
    MOCK_METHOD0(dispatch_driver_request_swapinterval, void());
    MOCK_METHOD0(driver_requests_info, void());
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
    int width = 271828;
    mcla::MirNativeWindow anw(mock_driver_interpreter);

    ASSERT_NE(nullptr, anw.query);

    EXPECT_CALL(mock_driver_interpreter, driver_requests_info(NATIVE_WINDOW_WIDTH))
        .Times(1)
        .WillOnce(Return(width));

    int returned_width;
    EXPECT_NO_THROW({
        anw->query(&anw, NATIVE_WINDOW_WIDTH ,&returned_width);
    });
    
    EXPECT_EQ(width, returned_width);
}

/* perform hook tests */
TEST_F(AndroidNativeWindowTest, native_window_perform_hook_callable)
{
    int format = 4;

    mcla::MirNativeWindow anw(mock_driver_interpreter);

    EXPECT_CALL(mock_driver_interpreter, dispatch_driver_request_format(format))
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

    EXPECT_CALL(mock_driver_interpreter, dispatch_driver_request_swapinterval(swap))
        .Times(1);

    ASSERT_NE(nullptr, anw->setSwapInterval);
    EXPECT_NO_THROW({
        anw->setSwapInterval(&anw, swap);
    });
}

/* dequeue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_dequeue_hook_callable)
{
    ANativeWindowBuffer* tmp;

    mcla::MirNativeWindow anw(mock_driver_interpreter);

    ASSERT_NE(nullptr, anw->dequeueBuffer);
    EXPECT_NO_THROW({
        anw->dequeueBuffer(anw, &tmp, -1);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_deprecated_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp;

    ASSERT_NE(nullptr, anw->dequeueBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        anw->dequeueBuffer_DEPRECATED(anw, &tmp);
    });
}


/* queue hook tests */
TEST_F(AndroidNativeWindowTest, native_window_queue_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, anw.queueBuffer);
    EXPECT_NO_THROW({
        anw->queueBuffer(&anw, tmp);
    });
}

TEST_F(AndroidNativeWindowTest, native_window_queue_deprecated_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, anw.queueBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        anw->queueBuffer_DEPRECATED(&anw, tmp);
    });
}

/* cancel hook tests */
TEST_F(AndroidNativeWindowTest, native_window_cancel_hooks_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, anw.cancelBuffer_DEPRECATED);
    ASSERT_NE(nullptr, anw.cancelBuffer);
    EXPECT_NO_THROW({
        anw->cancelBuffer_DEPRECATED(&anw, tmp);
        anw->cancelBuffer(&anw, tmp);
    });
}

/* lock hook tests */
TEST_F(AndroidNativeWindowTest, native_window_lock_hook_callable)
{
    mcla::MirNativeWindow anw(mock_driver_interpreter);
    ANativeWindowBuffer* tmp = 0x0;

    ASSERT_NE(nullptr, anw.lockBuffer_DEPRECATED);
    EXPECT_NO_THROW({
        anw->lockBuffer_DEPRECATED(&anw, tmp);
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



#if 0
//tests that should go somewhere else
TEST_F(AndroidNativeWindowTest, native_window_dequeue_calls_surface_get_current)
{
    using namespace testing;
    ANativeWindow* anw;
    ANativeWindowBuffer* tmp;

    EXPECT_CALL(*mock_surface, get_current_buffer())
        .Times(1)
        .WillOnce(Return(mock_client_buffer));
    anw = new mcla::MirNativeWindow(mock_surface.get());

    anw->dequeueBuffer_DEPRECATED(anw, &tmp);

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_gets_native_handle_from_returned_buffer)
{
    using namespace testing;
    ANativeWindow* anw;
    native_handle_t handle;
    ANativeWindowBuffer buffer;
    buffer.handle = &handle;

    ANativeWindowBuffer* tmp;

    EXPECT_CALL(*mock_client_buffer, get_native_handle())
        .Times(1)
        .WillOnce(Return(&buffer));
    EXPECT_CALL(*mock_surface, get_current_buffer())
        .Times(1)
        .WillOnce(Return(mock_client_buffer));

    anw = new mcla::MirNativeWindow(mock_surface.get());

    anw->dequeueBuffer_DEPRECATED(anw, &tmp);

    EXPECT_EQ(tmp, &buffer);
    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_dequeue_has_proper_rc)
{
    using namespace testing;
    ANativeWindow* anw;

    ANativeWindowBuffer* tmp;

    anw = new mcla::MirNativeWindow(mock_surface.get());

    auto ret = anw->dequeueBuffer_DEPRECATED(anw, &tmp);
    EXPECT_EQ(0, ret);

    delete anw;
}


TEST_F(AndroidNativeWindowTest, native_window_queue_advances_buffer)
{
    using namespace testing;
    ANativeWindow* anw;
    ANativeWindowBuffer* tmp = 0x0;

    EXPECT_CALL(*mock_surface, next_buffer(_,_))
        .Times(1);
    anw = new mcla::MirNativeWindow(mock_surface.get());

    anw->queueBuffer_DEPRECATED(anw, tmp);

    delete anw;
}

/* format is an int that is set by the driver. these are not the HAL_PIXEL_FORMATS in android */
TEST_F(AndroidNativeWindowTest, native_window_perform_remembers_format)
{
    ANativeWindow* anw;
    int format = 945;

    anw = new mcla::MirNativeWindow(mock_surface.get());

    anw->perform(anw, NATIVE_WINDOW_SET_BUFFERS_FORMAT , format);

    int tmp_format = 0;
    anw->query(anw, NATIVE_WINDOW_FORMAT, &tmp_format);

    EXPECT_EQ(tmp_format, format);

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_hint_query_hook)
{
    using namespace testing;
    ANativeWindow* anw;
    /* transform hint is a bitmask of a few options for rotation/flipping buffer. a value
       of zero is no transform */
    int transform_hint_zero = 0;
    int value;

    anw = new mcla::MirNativeWindow(mock_surface.get());

    auto rc = anw->query(anw, NATIVE_WINDOW_TRANSFORM_HINT ,&value);

    EXPECT_EQ(0, rc);
    EXPECT_EQ(transform_hint_zero, value);

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_default_width_query_hook)
{
    using namespace testing;
    ANativeWindow* anw;
    int value;

    anw = new mcla::MirNativeWindow(mock_surface.get());

    auto rc = anw->query(anw, NATIVE_WINDOW_DEFAULT_WIDTH ,&value);

    EXPECT_EQ(0, rc);
    EXPECT_EQ(surf_params.width, value);

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_default_height_query_hook)
{
    using namespace testing;
    ANativeWindow* anw;
    int value;

    anw = new mcla::MirNativeWindow(mock_surface.get());

    auto rc = anw->query(anw, NATIVE_WINDOW_DEFAULT_HEIGHT ,&value);

    EXPECT_EQ(0, rc);
    EXPECT_EQ(surf_params.height, value);

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_width_query_hook)
{
    using namespace testing;
    ANativeWindow* anw;
    int value;

    anw = new mcla::MirNativeWindow(mock_surface.get());

    EXPECT_CALL(*mock_surface, get_parameters())
        .Times(1);

    auto rc = anw->query(anw, NATIVE_WINDOW_WIDTH ,&value);

    EXPECT_EQ(0, rc);
    EXPECT_EQ(surf_params.width, value);

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_height_query_hook)
{
    using namespace testing;
    ANativeWindow* anw;
    int value;

    anw = new mcla::MirNativeWindow(mock_surface.get());
    EXPECT_CALL(*mock_surface, get_parameters())
        .Times(1);

    auto rc = anw->query(anw, NATIVE_WINDOW_HEIGHT ,&value);

    EXPECT_EQ(0, rc);
    EXPECT_EQ(surf_params.height, value);

    delete anw;
}
#endif
