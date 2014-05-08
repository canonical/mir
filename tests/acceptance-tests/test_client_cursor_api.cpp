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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/graphics/cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/cursor_images.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_shared.h"
#include "mir_test/event_factory.h"
#include "mir_test/wait_condition.h"
#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test_framework/input_testing_client_configuration.h"
#include "mir_test_framework/declarative_placement_strategy.h"
#include "mir_test_framework/cross_process_sync.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <assert.h>

namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mis = mir::input::synthesis;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace
{

struct MockCursor : public mg::Cursor
{
    MOCK_METHOD1(show, void(mg::CursorImage const&));
    MOCK_METHOD0(hide, void());

    MOCK_METHOD1(move_to, void(geom::Point));
};

struct NamedCursorImage : public mg::CursorImage
{
    NamedCursorImage(std::string const& name)
        : cursor_name(name)
    {
    }

    void const* as_argb_8888() const { return nullptr; }
    geom::Size size() const { return geom::Size{}; }

    std::string const cursor_name;
};

struct StubCursorImages : public mg::CursorImages
{
   std::shared_ptr<mg::CursorImage> image(std::string const& name, geom::Size const& /* size */)
   {
       return std::make_shared<NamedCursorImage>(name);
   }
};

bool cursor_is_named(mg::CursorImage const& i, std::string const& name)
{
    auto image = dynamic_cast<NamedCursorImage const*>(&i);
    assert(image);
    
    return image->cursor_name == name;
}

MATCHER(DefaultCursorImage, "")
{
    return cursor_is_named(arg, "default");
}

MATCHER_P(CursorNamed, name, "")
{
    return cursor_is_named(arg, name);
}

struct CursorSettingClient : mtf::TestingClientConfiguration
{
    static char const* const mir_test_socket;

    std::string const client_name;

    mtf::CrossProcessSync set_cursor_complete;
    mtf::CrossProcessSync client_may_exit;
    
    std::function<void(MirSurface*)> const set_cursor;

    CursorSettingClient(std::string const& client_name,
                        mtf::CrossProcessSync const& cursor_ready_fence,
                        mtf::CrossProcessSync const& client_may_exit_fence,
                        std::function<void(MirSurface*)> const& set_cursor)
        : client_name(client_name),
          set_cursor_complete(cursor_ready_fence),
          client_may_exit(client_may_exit_fence),
          set_cursor(set_cursor)
    {
    }

    void exec() override
    {
        auto connection = mir_connect_sync(mir_test_socket,
                                           client_name.c_str());
        
        ASSERT_TRUE(connection != NULL);
        MirSurfaceParameters const request_params =
            {
                client_name.c_str(),
                // For this fixture, we force geometry on server side
                0, 0,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };
        auto surface = mir_connection_create_surface_sync(connection, &request_params);
        
        set_cursor(surface);
        set_cursor_complete.signal_ready();
        
        client_may_exit.wait_for_signal_ready_for();
        
        mir_surface_release_sync(surface);
        mir_connection_release(connection);
    }
};

char const* const CursorSettingClient::mir_test_socket = mtf::test_socket_file().c_str();


typedef unsigned ClientCount;
struct CursorTestServerConfiguration : mtf::InputTestingServerConfiguration
{
    std::shared_ptr<ms::PlacementStrategy> placement_strategy;
    mtf::CrossProcessSync client_ready_fence;
    mtf::CrossProcessSync client_may_exit_fence;
    int const number_of_clients;

    std::function<void(MockCursor&, mt::WaitCondition&)> const expect_cursor_states;
    std::function<void(CursorTestServerConfiguration*)> const synthesize_cursor_motion;
    
    MockCursor cursor;

    CursorTestServerConfiguration(mtf::SurfaceGeometries surface_geometries_by_name,
                                  mtf::SurfaceDepths surface_depths_by_name,
                                  mtf::CrossProcessSync client_ready_fence,
                                  mtf::CrossProcessSync client_may_exit_fence,
                                  ClientCount const number_of_clients,
                                  std::function<void(MockCursor&, mt::WaitCondition&)> const& expect_cursor_states,
                                  std::function<void(CursorTestServerConfiguration*)> const& synthesize_cursor_motion)
        : placement_strategy(
              std::make_shared<mtf::DeclarativePlacementStrategy>(InputTestingServerConfiguration::the_placement_strategy(),
                  surface_geometries_by_name, surface_depths_by_name)),
          client_ready_fence(client_ready_fence),
          client_may_exit_fence(client_may_exit_fence),
          number_of_clients(number_of_clients),
          expect_cursor_states(expect_cursor_states),
          synthesize_cursor_motion(synthesize_cursor_motion)
    {
    }
    
    std::shared_ptr<ms::PlacementStrategy> the_placement_strategy() override
    {
        return placement_strategy;
    }
    
    std::shared_ptr<mg::Cursor> the_cursor() override
    {
        return mt::fake_shared(cursor);
    }
    
    std::shared_ptr<mg::CursorImages> the_cursor_images() override
    {
        return std::make_shared<StubCursorImages>();
    }
    
    void inject_input()
    {
        using namespace ::testing;

        for (int i = 1; i < number_of_clients + 1; i++)
            EXPECT_EQ(i, client_ready_fence.wait_for_signal_ready_for());

        mt::WaitCondition expectations_satisfied;
        
        // Clear any states applied during server initialization.
        Mock::VerifyAndClearExpectations(&cursor);
        expect_cursor_states(cursor, expectations_satisfied);

        synthesize_cursor_motion(this);
        expectations_satisfied.wait_for_at_most_seconds(60);

        EXPECT_CALL(cursor, show(_)).Times(AnyNumber()); // Client shutdown
        for (int i = 0; i < number_of_clients; i++)
            client_may_exit_fence.signal_ready();
    }
};

}

// TODO: A lot of common code setup in these tests could be moved to 
// a fixture.
using TestClientCursorAPI = BespokeDisplayServerTestFixture;

// In this set we create a 1x1 client surface at the point (1,0). The client requests to disable the cursor
// over this surface. Since the cursor starts at (0,0) we when we move the cursor by (1,0) thus causing it
// to enter the bounds of the first surface, we should observe it being disabled.
// TODO: Enable
TEST_F(TestClientCursorAPI, DISABLED_client_may_disable_cursor_over_surface)
{
    using namespace ::testing;

    std::string const test_client_name = "1";
    mtf::SurfaceGeometries client_geometries;
    client_geometries[test_client_name] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                          geom::Size{geom::Width{1}, geom::Height{1}}};
    
    mtf::CrossProcessSync client_ready_fence, client_may_exit_fence;

    CursorTestServerConfiguration server_conf(
        client_geometries, mtf::SurfaceDepths(),
        client_ready_fence, client_may_exit_fence,
        ClientCount{1},
        [](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            EXPECT_CALL(cursor, hide()).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        },
        [](CursorTestServerConfiguration *server)
        {
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
        });
    launch_server_process(server_conf);
    
    CursorSettingClient client_conf(test_client_name, client_ready_fence, client_may_exit_fence,
        [](MirSurface *surface)
        {
            // Disable cursor
            mir_wait_for(mir_surface_configure_cursor(surface, 
                mir_cursor_configuration_from_name(mir_disabled_cursor_name)));
        });
    launch_client_process(client_conf);
}

// TODO: Enable
TEST_F(TestClientCursorAPI, DISABLED_cursor_restored_when_leaving_surface)
{
    using namespace ::testing;

    std::string const test_client_name = "1";
    mtf::SurfaceGeometries client_geometries;
    client_geometries[test_client_name] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                          geom::Size{geom::Width{1}, geom::Height{1}}};
    
    mtf::CrossProcessSync client_ready_fence, client_may_exit_fence;

    CursorTestServerConfiguration server_conf(
        client_geometries, mtf::SurfaceDepths(),
        client_ready_fence, client_may_exit_fence,
        ClientCount{1},
        [](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            InSequence seq;
            EXPECT_CALL(cursor, hide()).Times(1);
            EXPECT_CALL(cursor, show(DefaultCursorImage())).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        },
        [](CursorTestServerConfiguration *server)
        {
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(2,0));
        });
    launch_server_process(server_conf);
    
    CursorSettingClient client_conf(test_client_name, client_ready_fence, client_may_exit_fence,
        [](MirSurface *surface)
        {
            // Disable cursor
            mir_wait_for(mir_surface_configure_cursor(surface, 
                mir_cursor_configuration_from_name(mir_disabled_cursor_name)));
        });
    launch_client_process(client_conf);
}

// TODO: Enable
TEST_F(TestClientCursorAPI, DISABLED_cursor_changed_when_crossing_surface_boundaries)
{
    using namespace ::testing;

    static std::string const test_client_name_1 = "1";
    static std::string const test_client_name_2 = "2";
    static std::string const client_1_cursor = test_client_name_1;
    static std::string const client_2_cursor = test_client_name_2;

    mtf::SurfaceGeometries client_geometries;
    client_geometries[test_client_name_1] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                          geom::Size{geom::Width{1}, geom::Height{1}}};
    client_geometries[test_client_name_2] = geom::Rectangle{geom::Point{geom::X{2}, geom::Y{0}},
                                                            geom::Size{geom::Width{1}, geom::Height{1}}};
    
    mtf::CrossProcessSync client_ready_fence, client_may_exit_fence;

    CursorTestServerConfiguration server_conf(
        client_geometries, mtf::SurfaceDepths(),
        client_ready_fence, client_may_exit_fence,
        ClientCount{2},
        [](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            InSequence seq;
            EXPECT_CALL(cursor, show(CursorNamed(client_1_cursor))).Times(1);
            EXPECT_CALL(cursor, show(CursorNamed(client_2_cursor))).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        },
        [](CursorTestServerConfiguration *server)
        {
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
        });
    launch_server_process(server_conf);
    
    CursorSettingClient client1_conf(test_client_name_1, client_ready_fence, client_may_exit_fence,
        [](MirSurface *surface)
        {
            mir_wait_for(mir_surface_configure_cursor(surface, mir_cursor_configuration_from_name(client_1_cursor.c_str())));
        });
    launch_client_process(client1_conf);
    CursorSettingClient client2_conf(test_client_name_2, client_ready_fence, client_may_exit_fence,
        [](MirSurface *surface)
        {
            // Disable cursor
            mir_wait_for(mir_surface_configure_cursor(surface, mir_cursor_configuration_from_name(client_2_cursor.c_str())));
        });
    launch_client_process(client2_conf);
}

// TODO: Enable
TEST_F(TestClientCursorAPI, DISABLED_cursor_request_taken_from_top_surface)
{
    using namespace ::testing;

    static std::string const test_client_name_1 = "1";
    static std::string const test_client_name_2 = "2";
    static std::string const client_1_cursor = test_client_name_1;
    static std::string const client_2_cursor = test_client_name_2;

    mtf::SurfaceGeometries client_geometries;
    client_geometries[test_client_name_1] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                          geom::Size{geom::Width{1}, geom::Height{1}}};
    client_geometries[test_client_name_1] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                          geom::Size{geom::Width{1}, geom::Height{1}}};
    mtf::SurfaceDepths client_depths;
    client_depths[test_client_name_1] = ms::DepthId{0};
    client_depths[test_client_name_2] = ms::DepthId{1};
    
    mtf::CrossProcessSync client_ready_fence, client_may_exit_fence;

    CursorTestServerConfiguration server_conf(
        client_geometries, client_depths,
        client_ready_fence, client_may_exit_fence,
        ClientCount{2},
        [](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            InSequence seq;
            EXPECT_CALL(cursor, show(CursorNamed(client_2_cursor))).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        },
        [](CursorTestServerConfiguration *server)
        {
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
        });
    launch_server_process(server_conf);
    
    CursorSettingClient client1_conf(test_client_name_1, client_ready_fence, client_may_exit_fence,
        [](MirSurface *surface)
        {
            mir_wait_for(mir_surface_configure_cursor(surface, mir_cursor_configuration_from_name(client_1_cursor.c_str())));
        });
    launch_client_process(client1_conf);
    CursorSettingClient client2_conf(test_client_name_2, client_ready_fence, client_may_exit_fence,
        [](MirSurface *surface)
        {
            mir_wait_for(mir_surface_configure_cursor(surface, mir_cursor_configuration_from_name(client_1_cursor.c_str())));
        });

    launch_client_process(client2_conf);
}

// TODO: Enable
TEST_F(TestClientCursorAPI, DISABLED_cursor_request_applied_without_cursor_motion)
{
    using namespace ::testing;
    static std::string const test_client_name_1 = "1";
    static std::string const client_1_cursor = test_client_name_1;

    mtf::SurfaceGeometries client_geometries;
    client_geometries[test_client_name_1] = geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}},
                                                          geom::Size{geom::Width{1}, geom::Height{1}}};
    
    mtf::CrossProcessSync client_ready_fence, client_may_exit_fence;
    static mtf::CrossProcessSync client_may_change_cursor;

    CursorTestServerConfiguration server_conf(
        client_geometries, mtf::SurfaceDepths(),
        client_ready_fence, client_may_exit_fence,
        ClientCount{1},
        [](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            InSequence seq;
            EXPECT_CALL(cursor, show(CursorNamed(client_1_cursor))).Times(1);
            EXPECT_CALL(cursor, hide()).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        },
        [](CursorTestServerConfiguration * /* server */)
        {
            client_may_change_cursor.signal_ready();
        });
    launch_server_process(server_conf);
    
    CursorSettingClient client1_conf(test_client_name_1, client_ready_fence, client_may_exit_fence,
        [&client_ready_fence](MirSurface *surface)
        {
            client_ready_fence.signal_ready();
            client_may_change_cursor.wait_for_signal_ready_for();
            mir_wait_for(mir_surface_configure_cursor(surface, mir_cursor_configuration_from_name(client_1_cursor.c_str())));
            mir_wait_for(mir_surface_configure_cursor(surface, 
                mir_cursor_configuration_from_name(mir_disabled_cursor_name)));
        });
    launch_client_process(client1_conf);
}
