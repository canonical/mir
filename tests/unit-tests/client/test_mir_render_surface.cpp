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

#include "src/client/mir_connection.h"
#include "src/client/default_connection_configuration.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "src/client/render_surface.h"
#include "src/client/connection_surface_map.h"
#include "src/client/mir_wait_handle.h"

#include "mir/client_platform_factory.h"
#include "mir/dispatch/dispatchable.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_client_buffer_factory.h"
#include "mir_protobuf.pb.h"

#include <sys/eventfd.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mclr = mcl::rpc;
namespace mp = mir::protobuf;
namespace md = mir::dispatch;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace testing;

namespace
{
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}

struct RenderSurfaceCallback
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    static void created(MirRenderSurface* render_surface, void *client_context)
    {
        auto const context = reinterpret_cast<RenderSurfaceCallback*>(client_context);
        context->invoked = true;
        context->resulting_render_surface = render_surface;
    }
    bool invoked = false;
    MirRenderSurface* resulting_render_surface = nullptr;
#pragma GCC diagnostic pop
};

struct MockRpcChannel : public mir::client::rpc::MirBasicRpcChannel,
                        public mir::dispatch::Dispatchable
{
    MockRpcChannel()
        : pollable_fd{eventfd(0, EFD_CLOEXEC)}
    {
        ON_CALL(*this, watch_fd()).WillByDefault(testing::Return(pollable_fd));
    }

    virtual void call_method(std::string const& name,
                    google::protobuf::MessageLite const* /*parameters*/,
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

    MOCK_METHOD0(discard_future_calls, void());
    MOCK_METHOD0(wait_for_outstanding_calls, void());

    MOCK_METHOD2(on_buffer_stream_create, void(mp::BufferStream&, google::protobuf::Closure* complete));

    MOCK_CONST_METHOD0(watch_fd, mir::Fd());
    MOCK_METHOD1(dispatch, bool(md::FdEvents));
    MOCK_CONST_METHOD0(relevant_events, md::FdEvents());
private:
    mir::Fd pollable_fd;
};

struct MockClientPlatform : public mcl::ClientPlatform
{
    MockClientPlatform()
    {
        auto native_window = std::make_shared<EGLNativeWindowType>();
        *native_window = reinterpret_cast<EGLNativeWindowType>(this);

        ON_CALL(*this, create_buffer_factory())
            .WillByDefault(Return(std::make_shared<mtd::StubClientBufferFactory>()));
        ON_CALL(*this, create_egl_native_window(_))
            .WillByDefault(Return(native_window));
    }

    void set_client_context(mcl::ClientContext* ctx)
    {
        client_context = ctx;
    }

    void populate(MirPlatformPackage& pkg) const override
    {
        client_context->populate_server_package(pkg);
    }

    MOCK_CONST_METHOD1(convert_native_buffer, MirNativeBuffer*(mir::graphics::NativeBuffer*));
    MOCK_CONST_METHOD0(platform_type, MirPlatformType());
    MOCK_METHOD1(platform_operation, MirPlatformMessage*(MirPlatformMessage const*));
    MOCK_METHOD0(create_buffer_factory, std::shared_ptr<mcl::ClientBufferFactory>());
    MOCK_METHOD2(use_egl_native_window, void(std::shared_ptr<void>, mcl::EGLNativeSurface*));
    MOCK_METHOD1(create_egl_native_window, std::shared_ptr<void>(mcl::EGLNativeSurface*));
    MOCK_METHOD0(create_egl_native_display, std::shared_ptr<EGLNativeDisplayType>());
    MOCK_CONST_METHOD2(get_egl_pixel_format, MirPixelFormat(EGLDisplay, EGLConfig));
    MOCK_METHOD2(request_interface, void*(char const*, int));
    MOCK_CONST_METHOD1(native_format_for, uint32_t(MirPixelFormat));
    MOCK_CONST_METHOD2(native_flags_for, uint32_t(MirBufferUsage, mir::geometry::Size));

    mcl::ClientContext* client_context = nullptr;
};

struct StubClientPlatformFactory : public mcl::ClientPlatformFactory
{
    StubClientPlatformFactory(std::shared_ptr<mcl::ClientPlatform> const& platform)
        : platform{platform}
    {
    }

    std::shared_ptr<mcl::ClientPlatform> create_client_platform(mcl::ClientContext*)
    {
        return platform;
    }

    std::shared_ptr<mcl::ClientPlatform> platform;
};

void connected_callback(MirConnection* /*connection*/, void* /*client_context*/)
{
}

class TestConnectionConfiguration : public mcl::DefaultConnectionConfiguration
{
public:
    TestConnectionConfiguration(
        std::shared_ptr<mcl::ClientPlatform> const& platform,
        std::shared_ptr<mclr::MirBasicRpcChannel> const& channel)
        : DefaultConnectionConfiguration(""),
          platform{platform},
          channel{channel}
    {
    }

    std::shared_ptr<mclr::MirBasicRpcChannel> the_rpc_channel() override
    {
        return channel;
    }

    std::shared_ptr<mcl::ClientPlatformFactory> the_client_platform_factory() override
    {
        return std::make_shared<StubClientPlatformFactory>(platform);
    }

private:
    std::shared_ptr<mcl::ClientPlatform> const platform;
    std::shared_ptr<mclr::MirBasicRpcChannel> const channel;
};
}

struct MirRenderSurfaceTest : public testing::Test
{
    MirRenderSurfaceTest()
        : mock_platform{std::make_shared<testing::NiceMock<MockClientPlatform>>()},
          mock_channel{std::make_shared<testing::NiceMock<MockRpcChannel>>()},
          conf{mock_platform, mock_channel},
          connection{std::make_shared<MirConnection>(conf)}
    {
        mock_platform->set_client_context(connection.get());
    }

    std::shared_ptr<testing::NiceMock<MockClientPlatform>> const mock_platform;
    std::shared_ptr<testing::NiceMock<MockRpcChannel>> const mock_channel;
    TestConnectionConfiguration conf;
    std::shared_ptr<MirConnection> const connection;
};

TEST_F(MirRenderSurfaceTest, render_surface_can_be_created_and_released)
{
    EXPECT_CALL(*mock_channel, on_buffer_stream_create(_,_))
        .WillOnce(Invoke([](mp::BufferStream& stream, google::protobuf::Closure*)
        {
            stream.mutable_id()->set_value(1);
        }));

    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirRenderSurface* render_surface_from_callback = nullptr;

    auto const render_surface_returned = connection->create_render_surface_with_content(
        {10, 10},
        reinterpret_cast<MirRenderSurfaceCallback>(assign_result),
        &render_surface_from_callback);
#pragma GCC diagnostic pop

    EXPECT_THAT(render_surface_from_callback, NotNull());
    EXPECT_THAT(render_surface_returned, NotNull());
    EXPECT_THAT(render_surface_from_callback, Eq(render_surface_returned));
    EXPECT_NO_THROW(connection->release_render_surface_with_content(render_surface_returned));

    connection->disconnect();
}

TEST_F(MirRenderSurfaceTest, creation_of_render_surface_creates_egl_native_window)
{
    RenderSurfaceCallback callback;

    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();

    EXPECT_CALL(*mock_platform, create_egl_native_window(nullptr));
    EXPECT_CALL(*mock_channel, on_buffer_stream_create(_,_))
        .WillOnce(Invoke([](mp::BufferStream& stream, google::protobuf::Closure*)
        {
            stream.mutable_id()->set_value(1);
        }));

    auto const render_surface =
        connection->create_render_surface_with_content({10, 10}, &RenderSurfaceCallback::created, &callback);

    EXPECT_THAT(render_surface, NotNull());
    EXPECT_TRUE(callback.invoked);
    EXPECT_THAT(callback.resulting_render_surface, NotNull());

    auto rs = connection->connection_surface_map()->render_surface(
        static_cast<void*>(callback.resulting_render_surface));

    EXPECT_TRUE(reinterpret_cast<mcl::RenderSurface*>(rs->valid()));
}

TEST_F(MirRenderSurfaceTest, render_surface_returns_connection)
{
    MirConnection* conn{ reinterpret_cast<MirConnection*>(0x12345678) };

    mcl::RenderSurface rs(
        conn, nullptr, nullptr, nullptr, {});

    EXPECT_THAT(rs.connection(), Eq(conn));
}

TEST_F(MirRenderSurfaceTest, render_surface_has_correct_id_before_content_creation)
{
    MirConnection* conn{ reinterpret_cast<MirConnection*>(0x12345678) };
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    mcl::RenderSurface rs(
        conn, nullptr, nullptr, mt::fake_shared(protobuf_bs), {});

    EXPECT_THAT(rs.stream_id().as_value(), Eq(id));
}

TEST_F(MirRenderSurfaceTest, render_surface_can_create_buffer_stream)
{
    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    auto native_window = mock_platform->create_egl_native_window(nullptr);

    mcl::RenderSurface rs(
        connection.get(), native_window, nullptr, mt::fake_shared(protobuf_bs), {});

    auto bs = rs.get_buffer_stream(2, 2, mir_pixel_format_abgr_8888,
        mir_buffer_usage_software);

    EXPECT_THAT(bs, NotNull());
}

TEST_F(MirRenderSurfaceTest, excepts_on_creation_of_buffer_stream_more_than_once)
{
    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    auto native_window = mock_platform->create_egl_native_window(nullptr);

    mcl::RenderSurface rs(
        connection.get(), native_window, mock_platform, mt::fake_shared(protobuf_bs), {});

    EXPECT_CALL(*mock_platform, use_egl_native_window(native_window,_));

    auto bs = rs.get_buffer_stream(2, 2, mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware);

    EXPECT_THROW(
        { rs.get_buffer_stream(2, 2, mir_pixel_format_abgr_8888,
              mir_buffer_usage_hardware); },
        std::logic_error);
    EXPECT_THAT(bs, NotNull());
}

TEST_F(MirRenderSurfaceTest, render_surface_creation_of_buffer_stream_with_hardware_usage_installs_new_native_window)
{
    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    auto native_window = mock_platform->create_egl_native_window(nullptr);

    mcl::RenderSurface rs(
        connection.get(), native_window, mock_platform, mt::fake_shared(protobuf_bs), {});

    EXPECT_CALL(*mock_platform, use_egl_native_window(native_window,_));

    auto bs = rs.get_buffer_stream(2, 2, mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware);

    EXPECT_THAT(bs, NotNull());
}

TEST_F(MirRenderSurfaceTest, render_surface_creation_of_buffer_stream_with_software_usage_does_not_install_new_native_window)
{
    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    auto native_window = mock_platform->create_egl_native_window(nullptr);

    mcl::RenderSurface rs(
        connection.get(), native_window, mock_platform, mt::fake_shared(protobuf_bs), {});

    EXPECT_CALL(*mock_platform, use_egl_native_window(_,_)).Times(0);

    auto bs = rs.get_buffer_stream(2, 2, mir_pixel_format_abgr_8888,
        mir_buffer_usage_software);

    EXPECT_THAT(bs, NotNull());
}

TEST_F(MirRenderSurfaceTest, render_surface_object_is_invalid_after_creation_exception)
{
    RenderSurfaceCallback callback;

    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();

    EXPECT_CALL(*mock_channel, on_buffer_stream_create(_,_))
        .WillOnce(DoAll(
            Invoke([](mp::BufferStream&, google::protobuf::Closure* c){ c->Run(); }),
            Throw(std::runtime_error("Eeek!"))));

    auto const render_surface =
        connection->create_render_surface_with_content({10, 10}, &RenderSurfaceCallback::created, &callback);

    EXPECT_TRUE(callback.invoked);
    EXPECT_THAT(callback.resulting_render_surface, NotNull());
    EXPECT_THAT(callback.resulting_render_surface, Eq(render_surface));

    auto rs = connection->connection_surface_map()->render_surface(callback.resulting_render_surface);

    EXPECT_THAT(rs->get_error_message(),
        StrEq("Error creating MirRenderSurface: no ID in response (disconnected?)"));
    EXPECT_FALSE(reinterpret_cast<mcl::RenderSurface*>(rs->valid()));
}

TEST_F(MirRenderSurfaceTest, render_surface_can_create_presentation_chain)
{
    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    auto native_window = mock_platform->create_egl_native_window(nullptr);

    mcl::RenderSurface rs(
        connection.get(), native_window, nullptr, mt::fake_shared(protobuf_bs), {});

    auto pc = rs.get_presentation_chain();

    EXPECT_THAT(pc, NotNull());
}

TEST_F(MirRenderSurfaceTest, excepts_on_creation_of_presentation_chain_more_than_once)
{
    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    auto native_window = mock_platform->create_egl_native_window(nullptr);

    mcl::RenderSurface rs(
        connection.get(), native_window, mock_platform, mt::fake_shared(protobuf_bs), {});

    auto pc = rs.get_presentation_chain();
    EXPECT_THAT(pc, NotNull());

    EXPECT_THROW(
        { rs.get_presentation_chain(); },
        std::logic_error);
}

TEST_F(MirRenderSurfaceTest, excepts_on_creation_of_chain_after_stream)
{
    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    auto native_window = mock_platform->create_egl_native_window(nullptr);

    mcl::RenderSurface rs(connection.get(),
                          native_window,
                          mock_platform,
                          mt::fake_shared(protobuf_bs),
                          {});

    auto bs = rs.get_buffer_stream(2, 2, mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware);

    EXPECT_THAT(bs, NotNull());
    EXPECT_THROW(
        { rs.get_presentation_chain(); },
        std::logic_error);
}

TEST_F(MirRenderSurfaceTest, excepts_on_creation_of_stream_after_chain)
{
    connection->connect("MirRenderSurfaceTest", connected_callback, 0)->wait_for_all();
    auto id = 123;

    mp::BufferStream protobuf_bs;
    mp::BufferStreamId bs_id;

    bs_id.set_value(id);
    *protobuf_bs.mutable_id() = bs_id;

    auto native_window = mock_platform->create_egl_native_window(nullptr);

    mcl::RenderSurface rs(connection.get(),
                          native_window,
                          mock_platform,
                          mt::fake_shared(protobuf_bs),
                          {});

    auto pc = rs.get_presentation_chain();

    EXPECT_THAT(pc, NotNull());
    EXPECT_THROW(
        { rs.get_buffer_stream(2, 2,
                               mir_pixel_format_abgr_8888,
                               mir_buffer_usage_hardware); },
        std::logic_error);
}
