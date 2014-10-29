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

#include "mir/options/option.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/surface.h"
#include "src/server/scene/session_container.h"
#include "mir/shell/surface_coordinator_wrapper.h"

#include "mir_test/wait_condition.h"
#include "mir_test/client_event_matchers.h"
#include "mir_test/barrier.h"
#include "mir_test_framework/deferred_in_process_server.h"
#include "mir_test_framework/stubbed_server_configuration.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;

namespace mtf = mir_test_framework;

namespace
{
class SimpleConfigurablePlacementStrategy : public ms::PlacementStrategy
{
public:
    ms::SurfaceCreationParameters place(ms::Session const& /*session*/,
                                        ms::SurfaceCreationParameters const& request_parameters) override
    {
        auto placed = request_parameters;
        placed.top_left = placement.top_left;
        placed.size = placement.size;
        return placed;
    }

    mir::geometry::Rectangle placement;
};

class SurfacePlacingConfiguration : public mtf::StubbedServerConfiguration
{
public:
    SurfacePlacingConfiguration()
        : placement_strategy{std::make_shared<SimpleConfigurablePlacementStrategy>()}
    {
    }

    std::shared_ptr<ms::PlacementStrategy> the_placement_strategy() override
    {
        return placement_strategy;
    }

    void set_surface_placement(mir::geometry::Rectangle const& where)
    {
        placement_strategy->placement = where;
    }

    bool is_debug_server()
    {
        return the_options()->is_set("debug");
    }

private:
    std::shared_ptr<SimpleConfigurablePlacementStrategy> placement_strategy;
};

char const* debugenv = "MIR_SERVER_DEBUG";

void dont_kill_me_bro(MirConnection* /*unused*/, MirLifecycleState /*unused*/, void* /*unused*/)
{
}

class TestDebugAPI : public mtf::DeferredInProcessServer
{
public:
    TestDebugAPI()
        : old_debug_env{::getenv(debugenv)}
    {
        mir::geometry::Rectangle surface_location;
        surface_location.top_left.x = mir::geometry::X{0};
        surface_location.top_left.y = mir::geometry::Y{0};
        surface_location.size.width = mir::geometry::Width{100};
        surface_location.size.height = mir::geometry::Height{100};

        server_configuration.set_surface_placement(surface_location);
    }

    ~TestDebugAPI()
    {
        ::unsetenv(debugenv);
        if (old_debug_env)
        {
            ::setenv(debugenv, old_debug_env, 1);
        }
    }

    void start_server_with_debug(bool debug)
    {
        if (debug)
        {
            ::setenv(debugenv, "", 1);
        }
        else
        {
            ::unsetenv(debugenv);
        }

        if (server_configuration.is_debug_server() != debug)
        {
            throw std::runtime_error{"Failed to set the debug flag correctly. Have you overriden this with --debug?"};
        }

        start_server();
        connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
        if (!mir_connection_is_valid(connection))
        {
            throw std::runtime_error{std::string{"Failed to connect to test server:"} +
                                     mir_connection_get_error_message(connection)};
        }
        mir_connection_set_lifecycle_event_callback(connection, dont_kill_me_bro, nullptr);
    }

    void TearDown() override
    {
        if (connection)
        {
            mir_connection_release(connection);
        }
        DeferredInProcessServer::TearDown();
    }

    mir::DefaultServerConfiguration& server_config() override
    {
        return server_configuration;
    }

    SurfacePlacingConfiguration server_configuration;
    MirConnection* connection{nullptr};

private:
    char const* old_debug_env;
};
}

TEST_F(TestDebugAPI, TranslatesSurfaceCoordinatesToScreenCoordinates)
{
    start_server_with_debug(true);

    mir::geometry::Rectangle surface_location;
    surface_location.top_left.x = mir::geometry::X{200};
    surface_location.top_left.y = mir::geometry::Y{100};
    surface_location.size.width = mir::geometry::Width{800};
    surface_location.size.height = mir::geometry::Height{600};

    server_configuration.set_surface_placement(surface_location);

    ASSERT_TRUE(mir_connection_is_valid(connection));

    MirSurfaceParameters const creation_parameters = {
        __PRETTY_FUNCTION__,
        800, 600,
        mir_pixel_format_argb_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };
    auto surf = mir_connection_create_surface_sync(connection, &creation_parameters);
    ASSERT_TRUE(mir_surface_is_valid(surf));

    int screen_x, screen_y, x, y;
    x = 35, y = 21;

    ASSERT_TRUE(mir_debug_surface_coords_to_screen(surf, x, y, &screen_x, &screen_y));
    EXPECT_EQ(x + surface_location.top_left.x.as_int(), screen_x);
    EXPECT_EQ(y + surface_location.top_left.y.as_int(), screen_y);

    mir_surface_release_sync(surf);

    surface_location.top_left.x = mir::geometry::X{100};
    surface_location.top_left.y = mir::geometry::Y{250};

    server_configuration.set_surface_placement(surface_location);

    surf = mir_connection_create_surface_sync(connection, &creation_parameters);
    ASSERT_TRUE(mir_surface_is_valid(surf));

    ASSERT_TRUE(mir_debug_surface_coords_to_screen(surf, x, y, &screen_x, &screen_y));
    EXPECT_EQ(x + surface_location.top_left.x.as_int(), screen_x);
    EXPECT_EQ(y + surface_location.top_left.y.as_int(), screen_y);

    mir_surface_release_sync(surf);
}

TEST_F(TestDebugAPI, ApiIsUnavaliableWhenServerNotStartedWithDebug)
{
    start_server_with_debug(false);

    MirSurfaceParameters const creation_parameters = {
        __PRETTY_FUNCTION__,
        800, 600,
        mir_pixel_format_argb_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };
    auto surf = mir_connection_create_surface_sync(connection, &creation_parameters);
    ASSERT_TRUE(mir_surface_is_valid(surf));

    int screen_x, screen_y;

    EXPECT_FALSE(mir_debug_surface_coords_to_screen(surf, 0, 0, &screen_x, &screen_y));

    mir_surface_release_sync(surf);
}
