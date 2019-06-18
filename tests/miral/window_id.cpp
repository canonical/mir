/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <mir/client/window_id.h>
#include <mir/client/window.h>
#include <mir/client/window_spec.h>

#include <miral/application_info.h>
#include <mir/client/connection.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_server.h"

using namespace testing;


struct WindowId : public miral::TestServer
{
    auto get_first_window(miral::WindowManagerTools& tools) -> miral::Window
    {
        auto app = tools.find_application([&](miral::ApplicationInfo const& /*info*/){return true;});
        auto app_info = tools.info_for(app);
        return app_info.windows().at(0);
    }
};

TEST_F(WindowId, server_can_identify_window_specified_by_client)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    using namespace mir::client;

    auto const connection = connect_client(test_name);
    auto const spec = WindowSpec::for_normal_window(connection, 50, 50)
        .set_name(test_name);

    Window const surface{spec.create_window()};

    mir::client::WindowId client_surface_id{surface};

    invoke_tools([&](miral::WindowManagerTools& tools)
        {
            auto const& window_info = tools.info_for_window_id(client_surface_id.c_str());

            ASSERT_TRUE(window_info.window());
            ASSERT_THAT(window_info.name(), Eq(test_name));
        });
}

TEST_F(WindowId, server_returns_correct_id_for_window)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    using namespace mir::client;

    auto const connection = connect_client(test_name);
    auto const spec = WindowSpec::for_normal_window(connection, 50, 50)
        .set_name(test_name);

    Window const surface{spec.create_window()};

    mir::client::WindowId client_surface_id{surface};

    invoke_tools([&](miral::WindowManagerTools& tools)
        {
            auto window = get_first_window(tools);
            auto id = tools.id_for_window(window);

            ASSERT_THAT(client_surface_id.c_str(), Eq(id));
        });
}

TEST_F(WindowId, server_fails_gracefully_to_identify_window_from_garbage_id)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    using namespace mir::client;

    auto const connection = connect_client(test_name);
    auto const spec = WindowSpec::for_normal_window(connection, 50, 50).set_name(test_name);

    Window const surface{spec.create_window()};

    mir::client::WindowId client_surface_id{surface};

    invoke_tools([](miral::WindowManagerTools& tools)
        {
            EXPECT_THROW(tools.info_for_window_id("garbage"), std::exception);
        });
}

TEST_F(WindowId, server_fails_gracefully_when_id_for_null_window_requested)
{
    invoke_tools([](miral::WindowManagerTools& tools)
        {
            miral::Window window;
            EXPECT_THROW(tools.id_for_window(window), std::runtime_error);
        });
}
