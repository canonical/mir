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

#include "mir/graphics/android/native_buffer.h"
#include "src/client/mir_client_surface.h"
#include "src/client/client_buffer.h"
#include "src/client/android/client_surface_interpreter.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test/fake_shared.h"
#include <system/window.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;

struct MockClientBuffer : public mcl::ClientBuffer
{
    MockClientBuffer()
    {
        using namespace testing;
        buffer = std::make_shared<mtd::StubAndroidNativeBuffer>();

        ON_CALL(*this, native_buffer_handle())
            .WillByDefault(Return(buffer));
    }
    ~MockClientBuffer() noexcept {}

    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, geom::PixelFormat());

    MOCK_CONST_METHOD0(age, uint32_t());
    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_METHOD0(increment_age, void());

    MOCK_CONST_METHOD0(native_buffer_handle, std::shared_ptr<mir::graphics::NativeBuffer>());

    std::shared_ptr<mir::graphics::NativeBuffer> buffer;
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
    MOCK_METHOD2(next_buffer, MirWaitHandle*(mir_surface_callback callback, void * context));
    MOCK_METHOD2(configure, MirWaitHandle*(MirSurfaceAttrib, int));
    MirSurfaceParameters params;
};

class AndroidInterpreterTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        surf_params.width = 530;
        surf_params.height = 715;
        surf_params.pixel_format = mir_pixel_format_abgr_8888;

        mock_client_buffer = std::make_shared<NiceMock<MockClientBuffer>>();
    }

    MirSurfaceParameters surf_params;
    std::shared_ptr<MockClientBuffer> mock_client_buffer;
};

TEST_F(AndroidInterpreterTest, native_window_dequeue_calls_surface_get_current)
{
    using namespace testing;
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1)
        .WillOnce(Return(mock_client_buffer));

    interpreter.driver_requests_buffer();
}

TEST_F(AndroidInterpreterTest, native_window_dequeue_gets_native_handle_from_returned_buffer)
{
    using namespace testing;
    auto buffer = std::make_shared<mtd::StubAndroidNativeBuffer>();

    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    EXPECT_CALL(*mock_client_buffer, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(buffer));
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1)
        .WillOnce(Return(mock_client_buffer));

    auto returned_buffer = interpreter.driver_requests_buffer();
    EXPECT_EQ(buffer.get(), returned_buffer);
}

TEST_F(AndroidInterpreterTest, native_window_queue_advances_buffer)
{
    using namespace testing;
    ANativeWindowBuffer buffer;

    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    EXPECT_CALL(mock_surface, next_buffer(_,_))
        .Times(1);

    interpreter.driver_returns_buffer(&buffer, -1);
}

/* format is an int that is set by the driver. these are not the HAL_PIXEL_FORMATS in android */
TEST_F(AndroidInterpreterTest, native_window_perform_remembers_format)
{
    int format = 945;
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    interpreter.dispatch_driver_request_format(format);
    auto tmp_format = interpreter.driver_requests_info(NATIVE_WINDOW_FORMAT);

    EXPECT_EQ(format, tmp_format);
}

TEST_F(AndroidInterpreterTest, native_window_hint_query_hook)
{
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);
    /* transform hint is a bitmask of a few options for rotation/flipping buffer. a value
       of zero is no transform */
    int transform_hint_zero = 0;
    auto transform = interpreter.driver_requests_info(NATIVE_WINDOW_TRANSFORM_HINT);

    EXPECT_EQ(transform_hint_zero, transform);
}

TEST_F(AndroidInterpreterTest, native_window_default_width_query_hook)
{
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    auto default_width = interpreter.driver_requests_info(NATIVE_WINDOW_DEFAULT_WIDTH);

    EXPECT_EQ(surf_params.width, default_width);
}

TEST_F(AndroidInterpreterTest, native_window_default_height_query_hook)
{
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    auto default_height = interpreter.driver_requests_info(NATIVE_WINDOW_DEFAULT_HEIGHT);

    EXPECT_EQ(surf_params.height, default_height);
}

TEST_F(AndroidInterpreterTest, native_window_width_query_hook)
{
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    auto width = interpreter.driver_requests_info(NATIVE_WINDOW_WIDTH);

    EXPECT_EQ(surf_params.width, width);
}

TEST_F(AndroidInterpreterTest, native_window_height_query_hook)
{
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    auto height = interpreter.driver_requests_info(NATIVE_WINDOW_HEIGHT);

    EXPECT_EQ(surf_params.height, height);
}

/* this is query key is a bit confusing from the system/window.h description.
   what it means is the minimum number of buffers that the server reserves for its own use in steady
   state. The drivers consider 'steady state' to begin after the first call to queueBuffer.
   So, for instance, if a driver requires 3 buffers to run at steady state, and the server needs
   to keep 2 buffers on hand at all time, the driver might dequeue 5 buffers, then cancel those 5 buffers.
   After the first call to queueBuffer however, the client may never own more than the number it has
   reserved (in this case, 3 buffers) */
TEST_F(AndroidInterpreterTest, native_window_minimum_undequeued_query_hook)
{
    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    auto num_buffers = interpreter.driver_requests_info(NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS);
    EXPECT_EQ(2, num_buffers);
}
