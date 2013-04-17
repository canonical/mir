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

#include "src/client/mir_client_surface.h"
#include "src/client/client_buffer.h"
#include "src/client/android/client_surface_interpreter.h"
#include <system/window.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

namespace
{
struct MockSyncFence : public mga::SyncObject
{
    ~MockSyncFence() noexcept {}
    MOCK_METHOD0(wait, void());
};
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

    MOCK_CONST_METHOD0(age, uint32_t());
    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_METHOD0(increment_age, void());

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
}

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
        mock_sync = std::make_shared<NiceMock<MockSyncFence>>();
    }

    MirSurfaceParameters surf_params;
    std::shared_ptr<MockClientBuffer> mock_client_buffer;
    std::shared_ptr<MockSyncFence> mock_sync;
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
    native_handle_t handle;
    ANativeWindowBuffer buffer;
    buffer.handle = &handle;

    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    EXPECT_CALL(*mock_client_buffer, get_native_handle())
        .Times(1)
        .WillOnce(Return(&buffer));
    EXPECT_CALL(mock_surface, get_current_buffer())
        .Times(1)
        .WillOnce(Return(mock_client_buffer));

    auto returned_buffer = interpreter.driver_requests_buffer();
    EXPECT_EQ(&buffer, returned_buffer);
}

TEST_F(AndroidInterpreterTest, native_window_queue_advances_buffer)
{
    using namespace testing;
    ANativeWindowBuffer buffer;

    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    EXPECT_CALL(mock_surface, next_buffer(_,_))
        .Times(1);

    interpreter.driver_returns_buffer(&buffer, mock_sync);
}

TEST_F(AndroidInterpreterTest, native_window_queue_waits_on_fence)
{
    using namespace testing;
    ANativeWindowBuffer buffer;

    testing::NiceMock<MockMirSurface> mock_surface{surf_params};
    mcla::ClientSurfaceInterpreter interpreter(mock_surface);

    EXPECT_CALL(*mock_sync, wait())
        .Times(1);

    interpreter.driver_returns_buffer(&buffer, mock_sync);
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
