/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/coordinate_translator.h"
#include "mir/shell/shell_wrapper.h"

#include "mir_test_framework/headless_test.h"
#include "mir_test_framework/any_surface.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"
#include "mir_toolkit/extensions/window_coordinate_translation.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace msh = mir::shell;

namespace mtf = mir_test_framework;
using namespace testing;

namespace
{
class SimpleConfigurablePlacementShell : public msh::ShellWrapper
{
public:
    using msh::ShellWrapper::ShellWrapper;

    auto create_surface(
        std::shared_ptr<ms::Session> const& session,
        ms::SurfaceCreationParameters const& params,
        std::shared_ptr<ms::SurfaceObserver> const& observer) -> std::shared_ptr<ms::Surface> override
    {
        auto const window = msh::ShellWrapper::create_surface(session, params, observer);
        window->move_to(placement.top_left);
        window->resize(placement.size);
        return window;
    }

    mir::geometry::Rectangle placement{{0, 0}, {100, 100}};
};

char const* const debugenv = "MIR_SERVER_DEBUG";
mir::geometry::Point const testpoint{13, 7};

void dont_kill_me_bro(MirConnection* /*unused*/, MirLifecycleState /*unused*/, void* /*unused*/)
{
}

class SimpleCoordinateTranslator : public mir::scene::CoordinateTranslator
{
public:
    mir::geometry::Point surface_to_screen(
        std::shared_ptr<mir::frontend::Surface> /*window*/,
        int32_t /*x*/,
        int32_t /*y*/) override
    {
        return testpoint;
    }
    bool translation_supported() const override
    {
        return true;
    }
};

class DebugAPI : public mtf::HeadlessTest
{
public:

    void SetUp() override
    {
        add_to_environment("MIR_SERVER_NO_FILE", "");

        server.wrap_shell([&](std::shared_ptr<msh::Shell> const& wrapped)
        {
            return placement_strategy =
                std::make_shared<SimpleConfigurablePlacementShell>(wrapped);
        });

        mtf::HeadlessTest::SetUp();
    }

    void set_surface_placement(mir::geometry::Rectangle const& where)
    {
        placement_strategy->placement = where;
    }

    void start_server_with_debug(bool debug)
    {
        if (debug)
        {
            add_to_environment(debugenv, "");
        }
        else
        {
            add_to_environment(debugenv, nullptr);
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
        placement_strategy.reset();
        if (connection)
        {
            mir_connection_release(connection);
        }

        stop_server();
        mtf::HeadlessTest::TearDown();
    }

    MirConnection* connection{nullptr};

private:
    std::shared_ptr<SimpleConfigurablePlacementShell> placement_strategy;
};
}

TEST_F(DebugAPI, translates_surface_coordinates_to_screen_coordinates_deprecated)
{
    start_server_with_debug(true);

    mir::geometry::Rectangle surface_location{{200, 100}, {800, 600}};

    set_surface_placement(surface_location);

    auto window = mtf::make_any_surface(connection);
    ASSERT_TRUE(mir_window_is_valid(window));

    int screen_x, screen_y, x, y;
    x = 35, y = 21;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    ASSERT_TRUE(mir_debug_surface_coords_to_screen(window, x, y, &screen_x, &screen_y));
#pragma GCC diagnostic pop
    EXPECT_EQ(x + surface_location.top_left.x.as_int(), screen_x);
    EXPECT_EQ(y + surface_location.top_left.y.as_int(), screen_y);

    mir_window_release_sync(window);

    surface_location.top_left = {100, 250};

    set_surface_placement(surface_location);

    window = mtf::make_any_surface(connection);
    ASSERT_TRUE(mir_window_is_valid(window));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    ASSERT_TRUE(mir_debug_surface_coords_to_screen(window, x, y, &screen_x, &screen_y));
#pragma GCC diagnostic pop
    EXPECT_EQ(x + surface_location.top_left.x.as_int(), screen_x);
    EXPECT_EQ(y + surface_location.top_left.y.as_int(), screen_y);

    mir_window_release_sync(window);
}

TEST_F(DebugAPI, is_unavailable_when_server_not_started_with_debug_deprecated)
{
    start_server_with_debug(false);

    auto window = mtf::make_any_surface(connection);
    ASSERT_TRUE(mir_window_is_valid(window));

    int screen_x, screen_y;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT_FALSE(mir_debug_surface_coords_to_screen(window, 0, 0, &screen_x, &screen_y));
#pragma GCC diagnostic pop

    mir_window_release_sync(window);
}

TEST_F(DebugAPI, is_overrideable_deprecated)
{
    server.override_the_coordinate_translator([&]()
        ->std::shared_ptr<mir::scene::CoordinateTranslator>
        {
            return std::make_shared<SimpleCoordinateTranslator>();
        });

    start_server_with_debug(false);

    auto window = mtf::make_any_surface(connection);
    ASSERT_TRUE(mir_window_is_valid(window));

    int screen_x, screen_y;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT_TRUE(mir_debug_surface_coords_to_screen(window, 0, 0, &screen_x, &screen_y));
#pragma GCC diagnostic pop
    EXPECT_EQ(testpoint.x.as_int(), screen_x);
    EXPECT_EQ(testpoint.y.as_int(), screen_y);

    mir_window_release_sync(window);
}

TEST_F(DebugAPI, translates_surface_coordinates_to_screen_coordinates)
{
    start_server_with_debug(true);

    mir::geometry::Rectangle window_location{{200, 100}, {800, 600}};

    set_surface_placement(window_location);

    auto window = mtf::make_any_surface(connection);
    ASSERT_TRUE(mir_window_is_valid(window));

    int screen_x, screen_y, x, y;
    x = 35, y = 21;

    auto ext = mir_extension_window_coordinate_translation_v1(connection);
    ASSERT_THAT(ext, Not(IsNull()));
    ext->window_translate_coordinates(window, x, y, &screen_x, &screen_y);
    EXPECT_EQ(x + window_location.top_left.x.as_int(), screen_x);
    EXPECT_EQ(y + window_location.top_left.y.as_int(), screen_y);

    mir_window_release_sync(window);
}

TEST_F(DebugAPI, is_unavailable_when_server_not_started_with_debug)
{
    start_server_with_debug(false);
    auto ext = mir_extension_window_coordinate_translation_v1(connection);
    EXPECT_THAT(ext, IsNull());
}

TEST_F(DebugAPI, is_overrideable)
{
    server.override_the_coordinate_translator([&]()
        ->std::shared_ptr<mir::scene::CoordinateTranslator>
        {
            return std::make_shared<SimpleCoordinateTranslator>();
        });

    start_server_with_debug(false);

    auto window = mtf::make_any_surface(connection);
    ASSERT_TRUE(mir_window_is_valid(window));

    int screen_x, screen_y;

    auto ext = mir_extension_window_coordinate_translation_v1(connection);
    ASSERT_THAT(ext, Not(IsNull()));
    ext->window_translate_coordinates(window, 0, 0, &screen_x, &screen_y);
    EXPECT_EQ(testpoint.x.as_int(), screen_x);
    EXPECT_EQ(testpoint.y.as_int(), screen_y);

    mir_window_release_sync(window);
}
