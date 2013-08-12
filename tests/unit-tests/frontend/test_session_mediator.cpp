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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/surfaces/buffer_stream.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/frontend/session_mediator_report.h"
#include "mir/frontend/session_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/shell/application_session.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_display.h"
#include "mir_test_doubles/mock_display_changer.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_display_changer.h"
#include "mir_test_doubles/mock_display.h"
#include "mir_test_doubles/mock_shell.h"
#include "mir_test_doubles/mock_frontend_surface.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/stub_session.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test/display_config_matchers.h"
#include "mir_test/fake_shared.h"
#include "mir/frontend/event_sink.h"
#include "mir/shell/surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
struct StubConfig : public mtd::NullDisplayConfig
{
    StubConfig(std::shared_ptr<mg::DisplayConfigurationOutput> const& conf)
       : outputs{conf, conf}
    {
    }
    virtual void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const override
    {
        for(auto const& disp : outputs)
        {
            f(*disp);
        }
    }

    std::vector<std::shared_ptr<mg::DisplayConfigurationOutput>> outputs;
};

struct MockConfig : public mg::DisplayConfiguration
{
    MOCK_CONST_METHOD1(for_each_card, void(std::function<void(mg::DisplayConfigurationCard const&)>));
    MOCK_CONST_METHOD1(for_each_output, void(std::function<void(mg::DisplayConfigurationOutput const&)>));
    MOCK_METHOD4(configure_output, void(mg::DisplayConfigurationOutputId, bool, geom::Point, size_t));
};

}

namespace
{
class StubbedSession : public mtd::StubSession
{
public:
    StubbedSession()
    {
        using namespace ::testing;

        mock_surface = std::make_shared<mtd::MockFrontendSurface>();
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>(geom::Size(), geom::Stride(), geom::PixelFormat());

        EXPECT_CALL(*mock_surface, size()).Times(AnyNumber()).WillRepeatedly(Return(geom::Size()));
        EXPECT_CALL(*mock_surface, pixel_format()).Times(AnyNumber()).WillRepeatedly(Return(geom::PixelFormat()));
        EXPECT_CALL(*mock_surface, advance_client_buffer()).Times(AnyNumber()).WillRepeatedly(Return(mock_buffer));

        EXPECT_CALL(*mock_surface, supports_input()).Times(AnyNumber()).WillRepeatedly(Return(true));
        EXPECT_CALL(*mock_surface, client_input_fd()).Times(AnyNumber()).WillRepeatedly(Return(testing_client_input_fd));
    }

    std::shared_ptr<mf::Surface> get_surface(mf::SurfaceId /* surface */) const
    {
        return mock_surface;
    }

    mtd::StubSurfaceBuilder surface_builder;
    std::shared_ptr<mtd::MockFrontendSurface> mock_surface;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    static int const testing_client_input_fd;
};

int const StubbedSession::testing_client_input_fd{11};

class MockGraphicBufferAllocator : public mg::GraphicBufferAllocator
{
public:
    MockGraphicBufferAllocator()
    {
        ON_CALL(*this, supported_pixel_formats())
            .WillByDefault(testing::Return(std::vector<geom::PixelFormat>()));
    }

    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const&)
    {
        return std::shared_ptr<mg::Buffer>();
    }

    MOCK_METHOD0(supported_pixel_formats, std::vector<geom::PixelFormat>());
    ~MockGraphicBufferAllocator() noexcept {}
};

class MockPlatform : public mg::Platform
{
 public:
    MockPlatform()
    {
        using namespace testing;
        ON_CALL(*this, create_buffer_allocator(_))
            .WillByDefault(Return(std::shared_ptr<mg::GraphicBufferAllocator>()));
        ON_CALL(*this, create_display(_))
            .WillByDefault(Return(std::make_shared<mtd::NullDisplay>()));
        ON_CALL(*this, get_ipc_package())
            .WillByDefault(Return(std::make_shared<mg::PlatformIPCPackage>()));
    }

    MOCK_METHOD1(create_buffer_allocator, std::shared_ptr<mg::GraphicBufferAllocator>(std::shared_ptr<mg::BufferInitializer> const&));
    MOCK_METHOD1(create_display,
                 std::shared_ptr<mg::Display>(
                     std::shared_ptr<mg::DisplayConfigurationPolicy> const&));
    MOCK_METHOD0(get_ipc_package, std::shared_ptr<mg::PlatformIPCPackage>());
    MOCK_METHOD0(create_internal_client, std::shared_ptr<mg::InternalClient>());
    MOCK_CONST_METHOD2(fill_ipc_package, void(std::shared_ptr<mg::BufferIPCPacker> const&,
                                              std::shared_ptr<mg::Buffer> const&));
};

struct SessionMediatorTest : public ::testing::Test
{
    SessionMediatorTest()
        : shell{std::make_shared<testing::NiceMock<mtd::MockShell>>()},
          graphics_platform{std::make_shared<testing::NiceMock<MockPlatform>>()},
          graphics_changer{std::make_shared<mtd::NullDisplayChanger>()},
          buffer_allocator{std::make_shared<testing::NiceMock<MockGraphicBufferAllocator>>()},
          report{std::make_shared<mf::NullSessionMediatorReport>()},
          resource_cache{std::make_shared<mf::ResourceCache>()},
          mediator{shell, graphics_platform, graphics_changer,
                   buffer_allocator, report, 
                   std::make_shared<mtd::NullEventSink>(),
                   resource_cache},
          stubbed_session{std::make_shared<StubbedSession>()},
          null_callback{google::protobuf::NewPermanentCallback(google::protobuf::DoNothing)}
    {
        using namespace ::testing;

        ON_CALL(*shell, open_session(_, _)).WillByDefault(Return(stubbed_session));
        ON_CALL(*shell, create_surface_for(_, _)).WillByDefault(Return(mf::SurfaceId{1}));
    }

    std::shared_ptr<testing::NiceMock<mtd::MockShell>> const shell;
    std::shared_ptr<MockPlatform> const graphics_platform;
    std::shared_ptr<msh::DisplayChanger> const graphics_changer;
    std::shared_ptr<testing::NiceMock<MockGraphicBufferAllocator>> const buffer_allocator;
    std::shared_ptr<mf::SessionMediatorReport> const report;
    std::shared_ptr<mf::ResourceCache> const resource_cache;
    mf::SessionMediator mediator;
    std::shared_ptr<StubbedSession> const stubbed_session;

    std::unique_ptr<google::protobuf::Closure> null_callback;
};
}

TEST_F(SessionMediatorTest, disconnect_releases_session)
{
    using namespace ::testing;

    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    EXPECT_CALL(*shell, close_session(_)).Times(1);

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());
    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
}

TEST_F(SessionMediatorTest, calling_methods_before_connect_throws)
{
    EXPECT_THROW({
        mp::SurfaceParameters request;
        mp::Surface response;

        mediator.create_surface(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::SurfaceId request;
        mp::Buffer response;

        mediator.next_buffer(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::SurfaceId request;

        mediator.release_surface(nullptr, &request, nullptr, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::DRMMagic request;
        mp::DRMAuthMagicStatus response;

        mediator.drm_auth_magic(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
    }, std::logic_error);
}

TEST_F(SessionMediatorTest, calling_methods_after_connect_works)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    {
        mp::SurfaceParameters request;
        mp::Surface response;

        mediator.create_surface(nullptr, &request, &response, null_callback.get());
    }
    {
        mp::SurfaceId request;
        mp::Buffer response;

        mediator.next_buffer(nullptr, &request, &response, null_callback.get());
    }
    {
        mp::SurfaceId request;

        mediator.release_surface(nullptr, &request, nullptr, null_callback.get());
    }

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
}

TEST_F(SessionMediatorTest, calling_methods_after_disconnect_throws)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());

    EXPECT_THROW({
        mp::SurfaceParameters surface_parameters;
        mp::Surface surface;

        mediator.create_surface(nullptr, &surface_parameters, &surface, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::SurfaceId request;
        mp::Buffer response;

        mediator.next_buffer(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::SurfaceId request;

        mediator.release_surface(nullptr, &request, nullptr, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::DRMMagic request;
        mp::DRMAuthMagicStatus response;

        mediator.drm_auth_magic(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
    }, std::logic_error);
}

TEST_F(SessionMediatorTest, can_reconnect_after_disconnect)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());
}

TEST_F(SessionMediatorTest, connect_packs_display_configuration)
{
    using namespace testing;
    geom::Size sz{1022, 2411};

    mtd::StubDisplayConfig config;

    auto mock_display = std::make_shared<mtd::MockDisplayChanger>();
    EXPECT_CALL(*mock_display, active_configuration())
        .Times(1)
        .WillOnce(Return(mt::fake_shared(config)));
    mf::SessionMediator mediator(
        shell, graphics_platform, mock_display,
        buffer_allocator, report, 
        std::make_shared<mtd::NullEventSink>(),
        resource_cache);

    mp::ConnectParameters connect_parameters;
    mp::Connection connection;
    connection.clear_platform();
    connection.clear_display_info();
    connection.clear_display_output();
    connection.clear_display_configuration();

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    EXPECT_THAT(connection.display_configuration(),
                mt::DisplayConfigMatches(std::cref(config)));
}

TEST_F(SessionMediatorTest, creating_surface_packs_response_with_input_fds)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    {
        mp::SurfaceParameters request;
        mp::Surface response;

        mediator.create_surface(nullptr, &request, &response, null_callback.get());
        EXPECT_EQ(StubbedSession::testing_client_input_fd, response.fd(0));
    }

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
}

TEST_F(SessionMediatorTest, no_input_channel_is_nonfatal)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;
    EXPECT_CALL(*stubbed_session->mock_surface, supports_input())
        .Times(1)
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*stubbed_session->mock_surface, client_input_fd())
        .Times(0);

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    {
        mp::SurfaceParameters request;
        mp::Surface response;

        mediator.create_surface(nullptr, &request, &response, null_callback.get());
    }

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
}

TEST_F(SessionMediatorTest, session_only_sends_needed_buffers)
{
    using namespace testing;

    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    {
        EXPECT_CALL(*stubbed_session->mock_buffer, id())
            .WillOnce(Return(mg::BufferID{4}))
            .WillOnce(Return(mg::BufferID{5}))
            .WillOnce(Return(mg::BufferID{4}))
            .WillOnce(Return(mg::BufferID{5}));

        mp::Surface surface_response;
        mp::SurfaceId buffer_request;
        mp::Buffer buffer_response[3];

        std::shared_ptr<mg::Buffer> tmp_buffer = stubbed_session->mock_buffer;
        EXPECT_CALL(*graphics_platform, fill_ipc_package(_, tmp_buffer))
            .Times(2);

        mp::SurfaceParameters surface_request;
        mediator.create_surface(nullptr, &surface_request, &surface_response, null_callback.get());
        mediator.next_buffer(nullptr, &buffer_request, &buffer_response[0], null_callback.get());
        mediator.next_buffer(nullptr, &buffer_request, &buffer_response[1], null_callback.get());
        mediator.next_buffer(nullptr, &buffer_request, &buffer_response[2], null_callback.get());
    }

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
}

TEST_F(SessionMediatorTest, buffer_resource_held_over_call)
{
    using namespace testing;

    auto stub_buffer1 = std::make_shared<mtd::StubBuffer>();
    auto stub_buffer2 = std::make_shared<mtd::StubBuffer>();

    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());
    mp::Surface surface_response;
    mp::SurfaceId buffer_request;
    mp::Buffer buffer_response;
    mp::SurfaceParameters surface_request;

    EXPECT_CALL(*stubbed_session->mock_surface, advance_client_buffer())
        .Times(2)
        .WillOnce(Return(stub_buffer1))
        .WillOnce(Return(stub_buffer2));
 
    auto refcount = stub_buffer1.use_count();
    mediator.create_surface(nullptr, &surface_request, &surface_response, null_callback.get());
    EXPECT_EQ(refcount+1, stub_buffer1.use_count());

    auto refcount2 = stub_buffer2.use_count();
    mediator.next_buffer(nullptr, &buffer_request, &buffer_response, null_callback.get());
    EXPECT_EQ(refcount, stub_buffer1.use_count());
    EXPECT_EQ(refcount2+1, stub_buffer2.use_count());

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
}

TEST_F(SessionMediatorTest, display_config_request)
{
    using namespace testing;
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    bool used0 = false, used1 = true;
    geom::Point pt0{44,22}, pt1{3,2};
    size_t mode_index0 = 1, mode_index1 = 3; 
    mg::DisplayConfigurationOutputId id0{6}, id1{3};

    NiceMock<MockConfig> mock_display_config;
    auto mock_display_selector = std::make_shared<mtd::MockDisplayChanger>();

    Sequence seq;
    EXPECT_CALL(*mock_display_selector, active_configuration())
        .InSequence(seq)
        .WillOnce(Return(mt::fake_shared(mock_display_config))); 
    EXPECT_CALL(*mock_display_selector, active_configuration())
        .InSequence(seq)
        .WillOnce(Return(mt::fake_shared(mock_display_config))); 
    EXPECT_CALL(mock_display_config, configure_output(id0, used0, pt0, mode_index0))
        .InSequence(seq);
    EXPECT_CALL(mock_display_config, configure_output(id1, used1, pt1, mode_index1))
        .InSequence(seq);
    EXPECT_CALL(*mock_display_selector, configure(_,_))
        .InSequence(seq);
 
    mf::SessionMediator session_mediator{
            shell, graphics_platform, mock_display_selector,
            buffer_allocator, report, std::make_shared<mtd::NullEventSink>(), resource_cache};

    session_mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mp::Void ignored;
    mp::DisplayConfiguration configuration; 
    auto disp0 = configuration.add_display_output();
    disp0->set_output_id(id0.as_value());
    disp0->set_used(used0);
    disp0->set_position_x(pt0.x.as_uint32_t());
    disp0->set_position_y(pt0.y.as_uint32_t());
    disp0->set_current_mode(mode_index0);

    auto disp1 = configuration.add_display_output();
    disp1->set_output_id(id1.as_value());
    disp1->set_used(used1);
    disp1->set_position_x(pt1.x.as_uint32_t());
    disp1->set_position_y(pt1.y.as_uint32_t());
    disp1->set_current_mode(mode_index1);

    session_mediator.configure_display(nullptr, &configuration, &ignored, null_callback.get());

    session_mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
}
