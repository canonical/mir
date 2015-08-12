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
#include "src/client/rpc/mir_display_server.h"

#include "mir/client_buffer_factory.h"
#include "mir/client_platform.h"

#include "mir/test/doubles/stub_client_buffer_stream_factory.h"
#include "mir/test/doubles/mock_client_buffer_stream_factory.h"
#include "mir/test/doubles/mock_client_buffer_stream.h"
#include "mir/test/doubles/null_client_buffer.h"
#include "mir/test/doubles/stub_display_server.h"
#include "mir/test/fake_shared.h"

#include <thread>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;
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

struct MockProtobufServer : mclr::DisplayServer
{
    MockProtobufServer() : mclr::DisplayServer(nullptr) {}
    MOCK_METHOD3(create_screencast,
                 void(mp::ScreencastParameters const* /*request*/,
                      mp::Screencast* /*response*/,
                      google::protobuf::Closure* /*done*/));
    MOCK_METHOD3(release_screencast,
                 void(mp::ScreencastId const* /*request*/,
                      mp::Void* /*response*/,
                      google::protobuf::Closure* /*done*/));
};

class StubProtobufServer : public mclr::DisplayServer
{
public:
    StubProtobufServer() : mclr::DisplayServer(nullptr) {}
    void create_screencast(
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
    arg1->clear_error();
    arg1->mutable_screencast_id()->set_value(screencast_id);
}

ACTION(SetCreateError)
{
    arg1->set_error("Test error");
}

ACTION(RunClosure)
{
    arg2->Run();
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
        make_consumer_stream(_,_,_,_,_)).WillByDefault(
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
                create_screencast(WithParams(default_region, default_size, default_pixel_format),_,_))
        .WillOnce(RunClosure());

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        nullptr,
        null_callback_func, nullptr};
}

TEST_F(MirScreencastTest, releases_screencast_on_release)
{
    using namespace testing;

    uint32_t const screencast_id{77};

    InSequence seq;

    EXPECT_CALL(mock_server,
                create_screencast(WithParams(default_region, default_size, default_pixel_format),_,_))
        .WillOnce(DoAll(SetCreateScreencastId(screencast_id), RunClosure()));

    EXPECT_CALL(mock_server,
                release_screencast(WithScreencastId(screencast_id),_,_))
        .WillOnce(RunClosure());

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format,
        mock_server,
        nullptr,
        null_callback_func, nullptr};

    screencast.release(null_callback_func, nullptr);
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
        nullptr,
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
        nullptr,
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
            nullptr,
            null_callback_func, nullptr);
    }, std::runtime_error);

    EXPECT_THROW({
        MirScreencast screencast(
            invalid_region,
            default_size,
            default_pixel_format, stub_server,
            nullptr,
            null_callback_func, nullptr);
    }, std::runtime_error);

    EXPECT_THROW({
        MirScreencast screencast(
            default_region,
            default_size,
            mir_pixel_format_invalid, stub_server,
            nullptr,
            null_callback_func, nullptr);
    }, std::runtime_error);
}

TEST_F(MirScreencastTest, is_invalid_if_server_create_screencast_fails)
{
    using namespace testing;

    EXPECT_CALL(mock_server, create_screencast(_,_,_))
        .WillOnce(DoAll(SetCreateError(), RunClosure()));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        nullptr,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    EXPECT_FALSE(screencast.valid());
}

TEST_F(MirScreencastTest, calls_callback_on_creation_failure)
{
    using namespace testing;

    MockCallback mock_cb;
    EXPECT_CALL(mock_server, create_screencast(_,_,_))
        .WillOnce(DoAll(SetCreateError(), RunClosure()));
    EXPECT_CALL(mock_cb, call(_,&mock_cb));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        nullptr,
        mock_callback_func, &mock_cb};

    screencast.creation_wait_handle()->wait_for_all();

    EXPECT_FALSE(screencast.valid());
}
