/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "src/client/render_surface.h"
#include "src/client/buffer_stream.h"
#include "src/client/default_connection_configuration.h"
#include "src/client/rpc/mir_display_server.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"

#include "mir/client_platform.h"
#include "mir_toolkit/mir_buffer_stream.h"

#include "mir_test_framework/stub_client_platform_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mp = mir::protobuf;
namespace mcl = mir::client;
namespace mclr = mcl::rpc;

using namespace testing;

struct BufferStreamCallback
{
    static void created(MirBufferStream* stream, void *client_context)
    {
        auto const context = reinterpret_cast<BufferStreamCallback*>(client_context);
        context->invoked = true;
        context->resulting_stream = stream;
    }
    bool invoked = false;
    MirBufferStream* resulting_stream = nullptr;
};

struct MockRpcChannel : public mclr::MirBasicRpcChannel
{
    virtual void call_method(std::string const& name,
                    google::protobuf::MessageLite const* /* parameters */,
                    google::protobuf::MessageLite* response,
                    google::protobuf::Closure* complete)
    {
        if (name == "create_buffer_stream")
        {
            auto response_message = static_cast<mp::BufferStream*>(response);
            on_buffer_stream_create(*response_message, complete);
        }
        complete->Run();
    }

    MOCK_METHOD2(on_buffer_stream_create, void(mp::BufferStream&, google::protobuf::Closure* complete));
};

class TestConnectionConfiguration : public mcl::DefaultConnectionConfiguration
{
public:
    TestConnectionConfiguration(
        std::shared_ptr<mclr::MirBasicRpcChannel> const& channel)
        : DefaultConnectionConfiguration(""),
          channel{channel}
    {
    }

    std::shared_ptr<mclr::MirBasicRpcChannel> the_rpc_channel() override
    {
        return channel;
    }

private:
    std::shared_ptr<mclr::MirBasicRpcChannel> const channel;
};

struct RenderSurfaceTest : public testing::Test
{
    std::shared_ptr<NiceMock<MockRpcChannel>> const
        mock_channel{ std::make_shared<NiceMock<MockRpcChannel>>() };
    TestConnectionConfiguration conf{ mock_channel };
    NiceMock<mclr::DisplayServer> mock_protobuf_server { mock_channel };
    std::shared_ptr<mcl::ClientPlatform> client_platform;
    std::shared_ptr<mcl::AsyncBufferFactory> async_buffer_factory;
    std::shared_ptr<mir::logging::Logger> mir_logger;
    std::shared_ptr<EGLNativeWindowType> native_window;
    MirConnection* connection{ nullptr };
};

TEST_F(RenderSurfaceTest, is_created_successfully)
{
    EXPECT_NO_THROW({
        mcl::RenderSurface rs(
            10, 10, mir_pixel_format_argb_8888, connection, mock_protobuf_server,
            conf.the_surface_map(), native_window, 2, client_platform,
            async_buffer_factory, mir_logger);
    });
}

TEST_F(RenderSurfaceTest, wait_handle_is_signalled_during_stream_creation_error)
{
    mcl::RenderSurface rs(
        10, 10, mir_pixel_format_argb_8888, connection, mock_protobuf_server,
        conf.the_surface_map(), native_window, 2, client_platform,
        async_buffer_factory, mir_logger);

    EXPECT_CALL(*mock_channel, on_buffer_stream_create(_,_))
        .WillOnce(Invoke([](mp::BufferStream& bs, google::protobuf::Closure*){ bs.set_error("No no no!"); }));
    EXPECT_FALSE(rs.create_client_buffer_stream(
        mir_buffer_usage_hardware, false, nullptr, nullptr)->is_pending());
}

TEST_F(RenderSurfaceTest, wait_handle_is_signalled_during_stream_creation_exception)
{
    mcl::RenderSurface rs(
        10, 10, mir_pixel_format_argb_8888, connection, mock_protobuf_server,
        conf.the_surface_map(), native_window, 2, client_platform,
        async_buffer_factory, mir_logger);

    EXPECT_CALL(*mock_channel, on_buffer_stream_create(_,_))
        .WillOnce(DoAll(
            Invoke([](mp::BufferStream&, google::protobuf::Closure* c){ c->Run(); }),
            Throw(std::runtime_error("exceptional exception!"))));

    auto wh = rs.create_client_buffer_stream(
        mir_buffer_usage_hardware, false, nullptr, nullptr);

    ASSERT_THAT(wh, Ne(nullptr));
    EXPECT_FALSE(wh->is_pending());
}

TEST_F(RenderSurfaceTest, callback_is_still_invoked_after_creation_error_and_error_stream_created)
{
    BufferStreamCallback callback;
    std::string error_msg = "Out of creative phrases";
    EXPECT_CALL(*mock_channel, on_buffer_stream_create(_,_))
        .WillOnce(Invoke([&](mp::BufferStream& bs, google::protobuf::Closure*)
        {
            bs.set_error(error_msg);
        }));

    mcl::RenderSurface rs(
        10, 10, mir_pixel_format_argb_8888, connection, mock_protobuf_server,
        conf.the_surface_map(), native_window, 2, client_platform,
        async_buffer_factory, mir_logger);

    rs.create_client_buffer_stream(
        mir_buffer_usage_hardware, false, &BufferStreamCallback::created, &callback);
    EXPECT_TRUE(callback.invoked);
    ASSERT_TRUE(callback.resulting_stream);
    EXPECT_THAT(mir_buffer_stream_get_error_message(callback.resulting_stream),
        StrEq("Error processing buffer stream response: " + error_msg));
}

TEST_F(RenderSurfaceTest, callback_is_still_invoked_after_creation_exception_and_error_stream_created)
{
    BufferStreamCallback callback;

    mcl::RenderSurface rs(
        10, 10, mir_pixel_format_argb_8888, connection, mock_protobuf_server,
        conf.the_surface_map(), native_window, 2, client_platform,
        async_buffer_factory, mir_logger);

    EXPECT_CALL(*mock_channel, on_buffer_stream_create(_,_))
        .WillOnce(DoAll(
            Invoke([](mp::BufferStream&, google::protobuf::Closure* c){ c->Run(); }),
            Throw(std::runtime_error("I'm exceptional"))));
    rs.create_client_buffer_stream(
        mir_buffer_usage_hardware, false, &BufferStreamCallback::created, &callback);

    EXPECT_TRUE(callback.invoked);
    ASSERT_TRUE(callback.resulting_stream);
    EXPECT_THAT(mir_buffer_stream_get_error_message(callback.resulting_stream),
        StrEq("Error processing buffer stream response: no ID in response (disconnected?)"));
}
