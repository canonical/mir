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
#include "mir/input/cursor_images.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_factory.h"
#include "mir/scene/null_observer.h"
#include "mir/scene/null_surface_observer.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test/barrier.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_shared.h"
#include "mir_test/wait_condition.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test_framework/testing_client_configuration.h"
#include "mir_test_framework/declarative_placement_strategy.h"
#include "mir_test_framework/deferred_in_process_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
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

    // We are not interested in mocking the motion in these tests as we
    // generate it ourself.
    void move_to(geom::Point)
    {
    }
};

struct NamedCursorImage : public mg::CursorImage
{
    NamedCursorImage(std::string const& name)
        : cursor_name(name)
    {
    }

    void const* as_argb_8888() const { return nullptr; }
    geom::Size size() const { return geom::Size{}; }
    geom::Displacement hotspot() const { return geom::Displacement{0, 0}; }

    std::string const cursor_name;
};

struct NamedCursorImages : public mi::CursorImages
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

struct ClientConfig : mtf::TestingClientConfiguration
{
    std::string connect_string;

    std::string const client_name;

    mt::Barrier& set_cursor_complete;
    mt::Barrier& client_may_exit;
    
    std::function<void(MirSurface*)> set_cursor;

    ClientConfig(std::string const& client_name,
                        mt::Barrier& cursor_ready_fence,
                        mt::Barrier& client_may_exit_fence)
        : client_name(client_name),
          set_cursor_complete(cursor_ready_fence),
          client_may_exit(client_may_exit_fence)
    {
    }
    
    virtual void thread_exec()
    {
        auto connection = mir_connect_sync(connect_string.c_str(),
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
        set_cursor_complete.ready();
        
        client_may_exit.ready();
        
        mir_surface_release_sync(surface);
        mir_connection_release(connection);
    }
    void tear_down() { if (thread.joinable()) thread.join(); }

    void exec() override
    {
        thread = std::thread([this]{ thread_exec(); });
    }
private:
    std::thread thread;
};

struct ServerConfiguration : mtf::InputTestingServerConfiguration
{
    mt::Barrier& cursor_configured_fence;
    mt::Barrier& client_may_exit_fence;
    
    std::function<void(MockCursor&, mt::WaitCondition&)> expect_cursor_states;
    std::function<void(ServerConfiguration*)> synthesize_cursor_motion;
    
    mtf::SurfaceGeometries client_geometries;
    mtf::SurfaceDepths client_depths;

    MockCursor cursor;
    
    ServerConfiguration(mt::Barrier& cursor_configured_fence, mt::Barrier& client_may_exit_fence)
        : cursor_configured_fence(cursor_configured_fence),
          client_may_exit_fence(client_may_exit_fence)
    {
    }

    std::shared_ptr<ms::PlacementStrategy> the_placement_strategy() override
    {
        return std::make_shared<mtf::DeclarativePlacementStrategy>(
            InputTestingServerConfiguration::the_placement_strategy(),
            client_geometries, client_depths);
    }

    std::shared_ptr<mg::Cursor> the_cursor() override
    {
        return mt::fake_shared(cursor);
    }
    
    std::shared_ptr<mi::CursorImages> the_cursor_images() override
    {
        return std::make_shared<NamedCursorImages>();
    }
    
    void inject_input()
    {
        using namespace ::testing;
        cursor_configured_fence.ready();
       
        mt::WaitCondition expectations_satisfied;

        // Clear any states applied during server initialization.
        Mock::VerifyAndClearExpectations(&cursor);
        expect_cursor_states(cursor, expectations_satisfied);
        
        // We are only interested in the cursor image changes, not
        // the synthetic motion.

        synthesize_cursor_motion(this);
        expectations_satisfied.wait_for_at_most_seconds(60);
        
        Mock::VerifyAndClearExpectations(&cursor);

        // Client shutdown
        EXPECT_CALL(cursor, show(_)).Times(AnyNumber());
        EXPECT_CALL(cursor, hide()).Times(AnyNumber());
        client_may_exit_fence.ready();
    }
    
};

struct TestClientCursorAPI : mtf::DeferredInProcessServer
{
    std::string const client_name_1 = "1";
    std::string const client_name_2 = "2";
    std::string const client_cursor_1 = "cursor-1";
    std::string const client_cursor_2 = "cursor-2";
    
    // Reset to higher values for more clients.
    mt::Barrier cursor_configured_fence{2};
    mt::Barrier client_may_exit_fence{2};
    
    ServerConfiguration server_configuration{cursor_configured_fence, client_may_exit_fence};
    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }
    
    ClientConfig client_config_1{client_name_1, cursor_configured_fence, client_may_exit_fence};
    ClientConfig client_config_2{client_name_2, cursor_configured_fence, client_may_exit_fence};

    // Default number allows one client.
    void set_client_count(unsigned count)
    {
        cursor_configured_fence.reset(count + 1);
        client_may_exit_fence.reset(count + 1);
    }

    void start_server()
    {
        DeferredInProcessServer::start_server();
        server_configuration.exec();
    }

    void start_client(ClientConfig& config)
    {
        config.connect_string = new_connection();
        config.exec();
    }

    void TearDown()
    {
        client_config_1.tear_down();
        client_config_2.tear_down();
        server_configuration.on_exit();
        DeferredInProcessServer::TearDown();
    }
};

}

// In this set we create a 1x1 client surface at the point (1,0). The client requests to disable the cursor
// over this surface. Since the cursor starts at (0,0) we when we move the cursor by (1,0) thus causing it
// to enter the bounds of the first surface, we should observe it being disabled.
TEST_F(TestClientCursorAPI, client_may_disable_cursor_over_surface)
{
    using namespace ::testing;

    server_configuration.client_geometries[client_name_1] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                                            geom::Size{geom::Width{1}, geom::Height{1}}};
    

    server_configuration.expect_cursor_states = [](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            EXPECT_CALL(cursor, hide()).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        };
    server_configuration.synthesize_cursor_motion = [](ServerConfiguration *server)
        {
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
        };
    start_server();
    
    client_config_1.set_cursor = [](MirSurface *surface)
        {
            auto conf = mir_cursor_configuration_from_name(mir_disabled_cursor_name);
            mir_wait_for(mir_surface_configure_cursor(surface, conf));
            mir_cursor_configuration_destroy(conf);
        };
    start_client(client_config_1);
}

TEST_F(TestClientCursorAPI, cursor_restored_when_leaving_surface)
{
    using namespace ::testing;

    server_configuration.client_geometries[client_name_1] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                                            geom::Size{geom::Width{1}, geom::Height{1}}};
    
    mtf::CrossProcessSync client_ready_fence, client_may_exit_fence;

    server_configuration.expect_cursor_states = [](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            InSequence seq;
            EXPECT_CALL(cursor, hide()).Times(1);
            EXPECT_CALL(cursor, show(DefaultCursorImage())).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        };
    server_configuration.synthesize_cursor_motion = [](ServerConfiguration *server)
        {
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(2,0));
        };
    start_server();
    

    client_config_1.set_cursor = [](MirSurface *surface)
        {
            // Disable cursor
            auto conf = mir_cursor_configuration_from_name(mir_disabled_cursor_name);
            mir_wait_for(mir_surface_configure_cursor(surface, conf));
            mir_cursor_configuration_destroy(conf);
        };
    start_client(client_config_1);
}

TEST_F(TestClientCursorAPI, cursor_changed_when_crossing_surface_boundaries)
{
    using namespace ::testing;

    server_configuration.client_geometries[client_name_1] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                                            geom::Size{geom::Width{1}, geom::Height{1}}};
    server_configuration.client_geometries[client_name_2] = geom::Rectangle{geom::Point{geom::X{2}, geom::Y{0}},
                                                                            geom::Size{geom::Width{1}, geom::Height{1}}};
    set_client_count(2);
    
    server_configuration.expect_cursor_states =
        [this](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            InSequence seq;
            EXPECT_CALL(cursor, show(CursorNamed(client_cursor_1))).Times(1);
            EXPECT_CALL(cursor, show(CursorNamed(client_cursor_2))).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        };
    server_configuration.synthesize_cursor_motion = 
        [](ServerConfiguration *server)
        {
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
        };
    start_server();
    
    client_config_1.set_cursor =
        [this](MirSurface *surface)
        {
            auto conf = mir_cursor_configuration_from_name(client_cursor_1.c_str());
            mir_wait_for(mir_surface_configure_cursor(surface, conf));
            mir_cursor_configuration_destroy(conf);
        };
    start_client(client_config_1);

    client_config_2.set_cursor = 
        [this](MirSurface *surface)
        {
            auto conf = mir_cursor_configuration_from_name(client_cursor_2.c_str());
            mir_wait_for(mir_surface_configure_cursor(surface, conf));
            mir_cursor_configuration_destroy(conf);
        };
    start_client(client_config_2);
}

TEST_F(TestClientCursorAPI, cursor_request_taken_from_top_surface)
{
    using namespace ::testing;

    server_configuration.client_geometries[client_name_1] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                                            geom::Size{geom::Width{1}, geom::Height{1}}};
    server_configuration.client_geometries[client_name_2] = geom::Rectangle{geom::Point{geom::X{1}, geom::Y{0}},
                                                                            geom::Size{geom::Width{1}, geom::Height{1}}};
    server_configuration.client_depths[client_name_1] = ms::DepthId{0};
    server_configuration.client_depths[client_name_2] = ms::DepthId{1};
    
    set_client_count(2);

    server_configuration.expect_cursor_states = 
        [this](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            InSequence seq;
            EXPECT_CALL(cursor, show(CursorNamed(client_cursor_2))).Times(1)
                .WillOnce(mt::WakeUp(&expectations_satisfied));
        };
    server_configuration.synthesize_cursor_motion = 
        [](ServerConfiguration *server)
        {
            server->fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 0));
        };
    start_server();
    
    
    client_config_1.set_cursor =
        [this](MirSurface *surface)
        {
            auto conf = mir_cursor_configuration_from_name(client_cursor_1.c_str());
            mir_wait_for(mir_surface_configure_cursor(surface, conf));
            mir_cursor_configuration_destroy(conf);
        };
    client_config_2.set_cursor =
        [this](MirSurface *surface)
        {
            auto conf = mir_cursor_configuration_from_name(client_cursor_2.c_str());
            mir_wait_for(mir_surface_configure_cursor(surface, conf));
            mir_cursor_configuration_destroy(conf);
        };
    start_client(client_config_1);
    start_client(client_config_2);
}

namespace
{

// In the following test the cursor changes are not responsive
// to cursor motion so we need a different synchronization model.
struct WaitsToChangeCursorClient : ClientConfig
{
    WaitsToChangeCursorClient(std::string const& client_name,
                              mt::Barrier& cursor_ready_fence,
                              mt::Barrier& client_may_exit_fence)
        : ClientConfig(client_name, cursor_ready_fence, client_may_exit_fence)
    {
    }

    void thread_exec() override
    {
        auto connection = mir_connect_sync(connect_string.c_str(),
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

        set_cursor_complete.ready();
        set_cursor(surface);
        
        client_may_exit.ready();
        
        mir_surface_release_sync(surface);
        mir_connection_release(connection);
    }
};

struct TestClientCursorAPINoMotion : TestClientCursorAPI
{
    mt::Barrier client_may_change_cursor{2};
    WaitsToChangeCursorClient waiting_client{client_name_1, cursor_configured_fence, client_may_exit_fence};
    
    void TearDown() override
    {
        waiting_client.tear_down();
        TestClientCursorAPI::TearDown();
    }
};

}

TEST_F(TestClientCursorAPINoMotion, cursor_request_applied_without_cursor_motion)
{
    using namespace ::testing;

    server_configuration.client_geometries[client_name_1] = 
        geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}},
                        geom::Size{geom::Width{1}, geom::Height{1}}};
    
    server_configuration.expect_cursor_states =
        [this](MockCursor& cursor, mt::WaitCondition& expectations_satisfied)
        {
            InSequence seq;
            EXPECT_CALL(cursor, show(CursorNamed(client_cursor_1))).Times(1);
            EXPECT_CALL(cursor, hide()).Times(1)
            .WillOnce(mt::WakeUp(&expectations_satisfied));
        };
    server_configuration.synthesize_cursor_motion =
        [this](ServerConfiguration * /* server */)
        {
            client_may_change_cursor.ready();
        };
    start_server();

    waiting_client.set_cursor = 
        [this](MirSurface *surface)
        {
            client_may_change_cursor.ready();
            auto conf1 = mir_cursor_configuration_from_name(client_cursor_1.c_str());
            auto conf2 = mir_cursor_configuration_from_name(mir_disabled_cursor_name);

            mir_wait_for(mir_surface_configure_cursor(surface, conf1));
            mir_wait_for(mir_surface_configure_cursor(surface, conf2));

            mir_cursor_configuration_destroy(conf1);
            mir_cursor_configuration_destroy(conf2);
        };
    start_client(waiting_client);
}

