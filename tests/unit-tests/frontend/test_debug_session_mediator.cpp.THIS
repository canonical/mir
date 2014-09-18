/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/server/frontend/debugging_session_mediator.h"
#include "mir/frontend/connector.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/frontend/resource_cache.h"
#include "src/server/scene/application_session.h"
#include "src/server/scene/basic_surface.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_session.h"
#include "mir_test_doubles/mock_shell.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/null_screencast.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_display_changer.h"

#include "mir_protobuf.pb.h"


#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mr = mir::report;

namespace
{
class StubbedDebuggableSession : public mtd::StubSession
{
public:
    StubbedDebuggableSession() : last_surface_id{1}
    {
        using namespace ::testing;

        mock_surface = std::make_shared<NiceMock<mtd::MockSurface>>();
        mock_surfaces[mf::SurfaceId{1}] = mock_surface;
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>(geom::Size(),
                                                                  geom::Stride(),
                                                                  MirPixelFormat());

        ON_CALL(*mock_surface, size())
            .WillByDefault(Return(geom::Size()));
        ON_CALL(*mock_surface, pixel_format())
            .WillByDefault(Return(MirPixelFormat()));
        ON_CALL(*mock_surface, advance_client_buffer())
            .WillByDefault(Return(mock_buffer));
    }

    std::shared_ptr<mf::Surface> get_surface(mf::SurfaceId surface) const
    {
        return mock_surfaces.at(surface);
    }

    mf::SurfaceId create_surface(ms::SurfaceCreationParameters const& /* params */) override
    {
        using namespace ::testing;
        auto id = mf::SurfaceId{last_surface_id};
        if (last_surface_id != 1) {
            mock_surfaces[id] = std::make_shared<NiceMock<mtd::MockSurface>>();
        }
        last_surface_id++;
        return id;
    }

    void destroy_surface(mf::SurfaceId surface) override
    {
        mock_surfaces.erase(surface);
    }

    std::shared_ptr<mtd::MockSurface> mock_surface;
    std::map<mf::SurfaceId, std::shared_ptr<mtd::MockSurface>> mock_surfaces;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    int last_surface_id;
};

struct MockConnector : public mf::Connector
{
public:
    void start() override {}
    void stop() override {}

    int client_socket_fd() const override { return 0; }

    MOCK_CONST_METHOD1(client_socket_fd, int (std::function<void(std::shared_ptr<mf::Session> const&)> const&));
};

struct DebugSessionMediatorTest : public ::testing::Test
{
    DebugSessionMediatorTest()
        : shell{std::make_shared<testing::NiceMock<mtd::MockShell>>()},
          graphics_platform{std::make_shared<mtd::NullPlatform>()},
          graphics_changer{std::make_shared<mtd::NullDisplayChanger>()},
          surface_pixel_formats{mir_pixel_format_argb_8888, mir_pixel_format_xrgb_8888},
          report{mr::null_session_mediator_report()},
          resource_cache{std::make_shared<mf::ResourceCache>()},
          stub_screencast{std::make_shared<mtd::NullScreencast>()},
          mediator{shell, graphics_platform, graphics_changer,
                   surface_pixel_formats, report,
                   std::make_shared<mtd::NullEventSink>(),
                   resource_cache, stub_screencast, &connector, {}},
          stubbed_session{std::make_shared<StubbedDebuggableSession>()},
          null_callback{google::protobuf::NewPermanentCallback(google::protobuf::DoNothing)}
    {
        using namespace ::testing;

        ON_CALL(*shell, open_session(_, _, _)).WillByDefault(Return(stubbed_session));
    }

    MockConnector connector;
    std::shared_ptr<testing::NiceMock<mtd::MockShell>> const shell;
    std::shared_ptr<mtd::NullPlatform> const graphics_platform;
    std::shared_ptr<mf::DisplayChanger> const graphics_changer;
    std::vector<MirPixelFormat> const surface_pixel_formats;
    std::shared_ptr<mf::SessionMediatorReport> const report;
    std::shared_ptr<mf::ResourceCache> const resource_cache;
    std::shared_ptr<mtd::NullScreencast> const stub_screencast;
    mf::DebuggingSessionMediator mediator;
    std::shared_ptr<StubbedDebuggableSession> const stubbed_session;

    std::unique_ptr<google::protobuf::Closure> null_callback;
};

}

TEST_F(DebugSessionMediatorTest, debug_surface_to_screen_looks_up_appropriate_surface)
{
    using namespace testing;

    mtd::StubBuffer buffer;
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());
    mp::SurfaceParameters surface_request;
    mp::Surface surface_response;

    mir::geometry::Point surface_location;
    surface_location.x = mir::geometry::X{256};
    surface_location.y = mir::geometry::Y{331};
    stubbed_session->mock_surface->move_to(surface_location);

    mediator.create_surface(nullptr, &surface_request, &surface_response, null_callback.get());
    mp::SurfaceId our_surface{surface_response.id()};

    mp::CoordinateTranslationRequest translate_request;
    mp::CoordinateTranslationResponse translate_response;
    translate_request.set_x(20);
    translate_request.set_y(40);
    *translate_request.mutable_surfaceid() = our_surface;

    mediator.translate_surface_to_screen(nullptr,
                                         &translate_request,
                                         &translate_response,
                                         null_callback.get());

    EXPECT_EQ(surface_location.x.as_int() + translate_request.x(),
              translate_response.x());
    EXPECT_EQ(surface_location.y.as_int() + translate_request.y(),
              translate_response.y());
}
