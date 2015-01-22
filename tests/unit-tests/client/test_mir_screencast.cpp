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

#include "mir_test_doubles/null_client_buffer.h"
#include "mir_test_doubles/stub_client_buffer_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace mtd = mir::test::doubles;

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
    MOCK_METHOD4(screencast_buffer,
                 void(google::protobuf::RpcController* /*controller*/,
                      mp::ScreencastId const* /*request*/,
                      mp::Buffer* /*response*/,
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

    void screencast_buffer(
        google::protobuf::RpcController* /*controller*/,
        mp::ScreencastId const* /*request*/,
        mp::Buffer* /*response*/,
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

struct StubEGLNativeWindowFactory : mcl::EGLNativeWindowFactory
{
    std::shared_ptr<EGLNativeWindowType>
        create_egl_native_window(mcl::ClientSurface*)
    {
        return std::make_shared<EGLNativeWindowType>(egl_native_window);
    }

    static EGLNativeWindowType egl_native_window;
};

EGLNativeWindowType StubEGLNativeWindowFactory::egl_native_window{
    reinterpret_cast<EGLNativeWindowType>(&StubEGLNativeWindowFactory::egl_native_window)};

struct MockClientBufferFactory : mcl::ClientBufferFactory
{
    MOCK_METHOD3(create_buffer,
                 std::shared_ptr<mcl::ClientBuffer>(
                    std::shared_ptr<MirBufferPackage> const& /*package*/,
                    mir::geometry::Size /*size*/, MirPixelFormat /*pf*/));
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

ACTION_P(SetCreateBufferId, buffer_id)
{
    arg2->mutable_buffer()->set_buffer_id(buffer_id);
}

ACTION_P(SetBufferId, buffer_id)
{
    arg2->set_buffer_id(buffer_id);
}

ACTION_P(SetCreateBufferFromPackage, package)
{
    arg2->clear_error();
    auto buffer = arg2->mutable_buffer();
    for (int i = 0; i != package.data_items; ++i)
    {
        buffer->add_data(package.data[i]);
    }

    for (int i = 0; i != package.fd_items; ++i)
    {
        buffer->add_fd(package.fd[i]);
    }

    buffer->set_stride(package.stride);
}

ACTION(SetCreateError)
{
    arg2->set_error("Test error");
}

ACTION(RunClosure)
{
    arg3->Run();
}

MATCHER_P(BufferPackageSharedPtrMatches, package, "")
{
    if (package.data_items != arg->data_items)
        return false;
    if (package.fd_items != arg->fd_items)
        return false;
    if (memcmp(package.data, arg->data, sizeof(package.data[0]) * package.data_items))
        return false;
    if (package.stride != arg->stride)
        return false;
    return true;
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
          stub_egl_native_window_factory{std::make_shared<StubEGLNativeWindowFactory>()},
          stub_client_buffer_factory{std::make_shared<mtd::StubClientBufferFactory>()},
          mock_client_buffer_factory{std::make_shared<MockClientBufferFactory>()}
    {
    }

    testing::NiceMock<MockProtobufServer> mock_server;
    StubProtobufServer stub_server;
    mir::geometry::Size default_size;
    mir::geometry::Rectangle default_region;
    MirPixelFormat default_pixel_format;
    std::shared_ptr<StubEGLNativeWindowFactory> const stub_egl_native_window_factory;
    std::shared_ptr<mtd::StubClientBufferFactory> const stub_client_buffer_factory;
    std::shared_ptr<MockClientBufferFactory> const mock_client_buffer_factory;
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
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
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
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
        null_callback_func, nullptr};

    screencast.release(null_callback_func, nullptr);
}

TEST_F(MirScreencastTest, requests_screencast_buffer_on_next_buffer)
{
    using namespace testing;
    uint32_t const screencast_id{77};

    InSequence seq;

    EXPECT_CALL(mock_server,
                create_screencast(_,WithParams(default_region, default_size, default_pixel_format),_,_))
        .WillOnce(DoAll(SetCreateScreencastId(screencast_id), RunClosure()));

    EXPECT_CALL(mock_server,
                screencast_buffer(_,WithScreencastId(screencast_id),_,_))
        .WillOnce(RunClosure());

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
        null_callback_func, nullptr};

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
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
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
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    MockCallback mock_cb;
    EXPECT_CALL(mock_cb, call(&screencast, &mock_cb));

    auto wh = screencast.release(mock_callback_func, &mock_cb);
    wh->wait_for_all();
}

TEST_F(MirScreencastTest, executes_callback_on_next_buffer)
{
    using namespace testing;

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    MockCallback mock_cb;
    EXPECT_CALL(mock_cb, call(&screencast, &mock_cb));

    auto wh = screencast.next_buffer(mock_callback_func, &mock_cb);
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
            stub_egl_native_window_factory,
            stub_client_buffer_factory,
            null_callback_func, nullptr);
    }, std::runtime_error);

    EXPECT_THROW({
        MirScreencast screencast(
            invalid_region,
            default_size,
            default_pixel_format, stub_server,
            stub_egl_native_window_factory,
            stub_client_buffer_factory,
            null_callback_func, nullptr);
    }, std::runtime_error);

    EXPECT_THROW({
        MirScreencast screencast(
            default_region,
            default_size,
            mir_pixel_format_invalid, stub_server,
            stub_egl_native_window_factory,
            stub_client_buffer_factory,
            null_callback_func, nullptr);
    }, std::runtime_error);
}

TEST_F(MirScreencastTest, returns_correct_surface_parameters)
{
    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    auto params = screencast.get_parameters();

    EXPECT_STREQ("", params.name);
    EXPECT_EQ(default_size.width.as_int(), params.width);
    EXPECT_EQ(default_size.height.as_int(), params.height);
    EXPECT_EQ(default_pixel_format, params.pixel_format);
    EXPECT_EQ(mir_buffer_usage_hardware, params.buffer_usage);
    EXPECT_EQ(mir_display_output_id_invalid, params.output_id);
}

TEST_F(MirScreencastTest, uses_buffer_message_from_server)
{
    using namespace testing;

    auto const client_buffer1 = std::make_shared<mtd::NullClientBuffer>();
    MirBufferPackage buffer_package;
    buffer_package.fd_items = 1;
    buffer_package.fd[0] = 16;
    buffer_package.data_items = 2;
    buffer_package.data[0] = 100;
    buffer_package.data[1] = 234;
    buffer_package.stride = 768;

    EXPECT_CALL(mock_server,
                create_screencast(_,WithParams(default_region, default_size, default_pixel_format),_,_))
        .WillOnce(DoAll(SetCreateBufferFromPackage(buffer_package), RunClosure()));

    EXPECT_CALL(*mock_client_buffer_factory,
                create_buffer(BufferPackageSharedPtrMatches(buffer_package),_,_))
        .WillOnce(Return(client_buffer1));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        stub_egl_native_window_factory,
        mock_client_buffer_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();
}

TEST_F(MirScreencastTest, returns_current_client_buffer)
{
    using namespace testing;

    uint32_t const screencast_id = 88;
    int const buffer_id1 = 5;
    int const buffer_id2 = 6;
    auto const client_buffer1 = std::make_shared<mtd::NullClientBuffer>();
    auto const client_buffer2 = std::make_shared<mtd::NullClientBuffer>();

    EXPECT_CALL(mock_server,
                create_screencast(_,WithParams(default_region, default_size, default_pixel_format),_,_))
        .WillOnce(DoAll(SetCreateBufferId(buffer_id1),
                        SetCreateScreencastId(screencast_id),
                        RunClosure()));

    EXPECT_CALL(mock_server,
                screencast_buffer(_,WithScreencastId(screencast_id),_,_))
        .WillOnce(DoAll(SetBufferId(buffer_id2), RunClosure()));

    EXPECT_CALL(*mock_client_buffer_factory, create_buffer(_,_,_))
        .WillOnce(Return(client_buffer1))
        .WillOnce(Return(client_buffer2));

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, mock_server,
        stub_egl_native_window_factory,
        mock_client_buffer_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    EXPECT_EQ(client_buffer1, screencast.get_current_buffer());

    auto wh = screencast.next_buffer(null_callback_func, nullptr);
    wh->wait_for_all();

    EXPECT_EQ(client_buffer2, screencast.get_current_buffer());
}

TEST_F(MirScreencastTest, gets_egl_native_window)
{
    using namespace testing;

    MirScreencast screencast{
        default_region,
        default_size,
        default_pixel_format, stub_server,
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
        null_callback_func, nullptr};

    screencast.creation_wait_handle()->wait_for_all();

    auto egl_native_window = screencast.egl_native_window();

    EXPECT_EQ(StubEGLNativeWindowFactory::egl_native_window, egl_native_window);
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
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
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
        stub_egl_native_window_factory,
        stub_client_buffer_factory,
        mock_callback_func, &mock_cb};

    screencast.creation_wait_handle()->wait_for_all();

    EXPECT_FALSE(screencast.valid());
}
