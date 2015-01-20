/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/client/mir_screencast.h"

#include "mir/client_buffer_factory.h"
#include "mir/client_platform.h"

#include "mir_test_doubles/stub_client_buffer_stream_factory.h"
#include "mir_test_doubles/mock_client_buffer_stream_factory.h"
#include "mir_test_doubles/mock_client_buffer_stream.h"
#include "mir_test_doubles/null_client_buffer.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace google
{
namespace protobuf
{
class RpcController;
}
}

namespace
{

struct MockProtobufServer : mir::protobuf::DisplayServer
{
    MOCK_METHOD4(create_screencast,
                 void(google::protobuf::RpcController* /*controller*/,
                      mp::ScreencastParameters const* /*request*/,
                      mp::Screencast* /*response*/,
                      google::protobuf::Closure* /*done*/));
    MOCK_METHOD4(release_screencast,
                 void(google::protobuf::RpcController* /*controller*/,
                      mp::ScreencastId const* /*request*/,
                      mp::Void* /*response*/,
                      google::protobuf::Closure* /*done*/));
};

class StubProtobufServer : public mir::protobuf::DisplayServer
{
public:
    void create_screencast(
        google::protobuf::RpcController* /*controller*/,
        mp::ScreencastParameters const* /*request*/,
        mp::Screencast* response,
        google::protobuf::Closure* done) override
    {
        if (server_thread.joinable())
            server_thread.join();
        server_thread = std::thread{
            [response, done, this]
            {
                response->clear_error();
                done->Run();
            }};
    }

    void release_screencast(
        google::protobuf::RpcController* /*controller*/,
        mp::ScreencastId const* /*request*/,
        mp::Void* /*response*/,
        google::protobuf::Closure* done) override
    {
        if (server_thread.joinable())
            server_thread.join();
        server_thread = std::thread{[done, this] { done->Run(); }};
    }

    ~StubProtobufServer()
    {
        if (server_thread.joinable())
            server_thread.join();
    }

private:
    std::thread server_thread;
};

MATCHER_P(WithOutputId, value, "")
{
    return arg->output_id() == value;
}

MATCHER_P3(WithParams, region, size, pixel_format, "")
{
    return arg->width()  == size.width.as_uint32_t() &&
           arg->height() == size.height.as_uint32_t() &&
           arg->region().left()   == region.top_left.x.as_int() &&
           arg->region().top()    == region.top_left.y.as_int() &&
           arg->region().width()  == region.size.width.as_uint32_t() &&
           arg->region().height() == region.size.height.as_uint32_t() &&
           arg->pixel_format() == pixel_format;
}

MATCHER_P(WithScreencastId, value, "")
{
    return arg->value() == value;
}

ACTION_P(SetCreateScreencastId, screencast_id)
{
    arg2->clear_error();
    arg2->mutable_screencast_id()->set_value(screencast_id);
}

ACTION(SetCreateError)
{
    arg2->set_error("Test error");
}

ACTION(RunClosure)
{
    arg3->Run();
}

struct MockCallback
{
    MOCK_METHOD2(call, void(void*, void*));
};

void mock_callback_func(MirScreencast* screencast, void* context)
{
    auto mock_cb = static_cast<MockCallback*>(context);
    mock_cb->call(screencast, context);
}

void null_callback_func(MirScreencast*, void*)
{
}



class MirScreencastTest : public testing::Test
{
public:
    MirScreencastTest()
        : default_size{1, 1},
          default_region{{0, 0}, {1, 1}},
          default_pixel_format{mir_pixel_format_xbgr_8888},
          stub_buffer_stream_factory{std::make_shared<mtd::StubClientBufferStreamFactory>()},
          mock_buffer_stream_factory{std::make_shared<mtd::MockClientBufferStreamFactory>()}
    {
        using namespace ::testing;

        ON_CALL(*mock_buffer_stream_factory,
        make_consumer_stream(_,_)).WillByDefault(
            Return(mt::fake_shared(mock_bs)));
    }

    testing::NiceMock<MockProtobufServer> mock_server;
    StubProtobufServer stub_server;
    mir::geometry::Size default_size;
    mir::geometry::Rectangle default_region;
    MirPixelFormat default_pixel_format;
    std::shared_ptr<mtd::StubClientBufferStreamFactory> const stub_buffer_stream_factory;
    std::shared_ptr<mtd::MockClientBufferStreamFactory> const mock_buffer_stream_factory;
    mtd::MockClientBufferStream mock_bs;
};

}

TEST_F(MirScreencastTest, creates_screencast_on_construction)
{
    using namespace testing;

    EXPECT_CALL(mock_server,
                create_screencast(_,WithParams(default_region, default_size, default_pixel_format),_,_))
        .WillOnce(RunClosure());

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        stub_buffer_stream_factory,
        null_callback_func, nullptr};
}

TEST_F(MirScreencastTest, releases_screencast_on_release)
{
    using namespace testing;

    uint32_t const screencast_id{77};

    InSequence seq;

    EXPECT_CALL(mock_server,
                create_screencast(_,WithParams(default_region, default_size, default_pixel_format),_,_))
        .WillOnce(DoAll(SetCreateScreencastId(screencast_id), RunClosure()));

    EXPECT_CALL(mock_server,
                release_screencast(_,WithScreencastId(screencast_id),_,_))
        .WillOnce(RunClosure());

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        stub_buffer_stream_factory,
        null_callback_func, nullptr};

    screencast.release(null_callback_func, nullptr);
}

TEST_F(MirScreencastTest, advances_buffer_stream_on_next_buffer)
{
    using namespace testing;

    InSequence seq;

    EXPECT_CALL(mock_bs, next_buffer(_)).Times(1)
        .WillOnce(Return(nullptr));
    
    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        mock_buffer_stream_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    screencast.next_buffer(null_callback_func, nullptr);
}

TEST_F(MirScreencastTest, executes_callback_on_creation)
{
    using namespace testing;

    MockCallback mock_cb;
    EXPECT_CALL(mock_cb, call(_, &mock_cb));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        stub_buffer_stream_factory,
        mock_callback_func, &mock_cb};

    screencast.creation_wait_handle()->wait_for_all();
}

TEST_F(MirScreencastTest, executes_callback_on_release)
{
    using namespace testing;

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        stub_buffer_stream_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    MockCallback mock_cb;
    EXPECT_CALL(mock_cb, call(&screencast, &mock_cb));

    auto wh = screencast.release(mock_callback_func, &mock_cb);
    wh->wait_for_all();
}

TEST_F(MirScreencastTest, construction_throws_on_invalid_params)
{
    mir::geometry::Size const invalid_size{0, 0};
    mir::geometry::Rectangle const invalid_region{{0, 0}, {0, 0}};

    EXPECT_THROW({
        MirScreencast screencast(
            default_region,
            invalid_size,
            default_pixel_format, stub_server,
            stub_buffer_stream_factory,
            null_callback_func, nullptr);
    }, std::runtime_error);

    EXPECT_THROW({
        MirScreencast screencast(
            invalid_region,
            default_size,
            default_pixel_format, stub_server,
            stub_buffer_stream_factory,
            null_callback_func, nullptr);
    }, std::runtime_error);

    EXPECT_THROW({
        MirScreencast screencast(
            default_region,
            default_size,
            mir_pixel_format_invalid, stub_server,
            stub_buffer_stream_factory,
            null_callback_func, nullptr);
    }, std::runtime_error);
}

TEST_F(MirScreencastTest, returns_params_from_buffer_stream)
{
    using namespace ::testing;

    MirSurfaceParameters expected_params;
    expected_params.width = 1732;
    expected_params.height = 33;
    expected_params.pixel_format = default_pixel_format;
    expected_params.buffer_usage = mir_buffer_usage_hardware;
    expected_params.output_id = mir_display_output_id_invalid;
    
    EXPECT_CALL(mock_bs, get_parameters()).Times(1)
        .WillOnce(Return(expected_params));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        mock_buffer_stream_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    auto params = screencast.get_parameters();
    EXPECT_EQ(expected_params.width, params.width);
    EXPECT_EQ(expected_params.height, params.height);
    EXPECT_EQ(expected_params.pixel_format, params.pixel_format);
    EXPECT_EQ(expected_params.buffer_usage, params.buffer_usage);
    EXPECT_EQ(expected_params.output_id, params.output_id);
}

TEST_F(MirScreencastTest, returns_current_client_buffer)
{
    using namespace testing;

    auto const expected_client_buffer = std::make_shared<mtd::NullClientBuffer>();

    EXPECT_CALL(mock_bs, get_current_buffer()).Times(1)
        .WillOnce(Return(expected_client_buffer));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        mock_buffer_stream_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    EXPECT_EQ(expected_client_buffer, screencast.get_current_buffer());
}

TEST_F(MirScreencastTest, gets_egl_native_window)
{
    using namespace testing;
    
    EGLNativeWindowType expected_native_window = reinterpret_cast<EGLNativeWindowType>(this); // any unique value
    EXPECT_CALL(mock_bs, egl_native_window()).Times(1)
        .WillOnce(Return(expected_native_window));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        mock_buffer_stream_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    auto egl_native_window = screencast.egl_native_window();

    EXPECT_EQ(expected_native_window, egl_native_window);
}

TEST_F(MirScreencastTest, is_invalid_if_server_create_screencast_fails)
{
    using namespace testing;

    EXPECT_CALL(mock_server, create_screencast(_,_,_,_))
        .WillOnce(DoAll(SetCreateError(), RunClosure()));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        stub_buffer_stream_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    EXPECT_FALSE(screencast.valid());
}

TEST_F(MirScreencastTest, calls_callback_on_creation_failure)
{
    using namespace testing;

    MockCallback mock_cb;
    EXPECT_CALL(mock_server, create_screencast(_,_,_,_))
        .WillOnce(DoAll(SetCreateError(), RunClosure()));
    EXPECT_CALL(mock_cb, call(_,&mock_cb));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        stub_buffer_stream_factory,
        mock_callback_func, &mock_cb};

    screencast.creation_wait_handle()->wait_for_all();

    EXPECT_FALSE(screencast.valid());
}
