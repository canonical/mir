/*
 * Copyright © 2017 Canonical Ltd.
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

#include <miral/window_manager_tools.h>

#include <mir/client/connection.h>
#include <mir/client/surface.h>
#include <mir/client/window.h>
#include <mir/client/window_spec.h>
#include <mir_toolkit/mir_buffer_stream.h>

#include "test_server.h"

#include <gmock/gmock.h>
#include <mir/test/signal.h>


using namespace testing;
using namespace mir::client;
using namespace std::chrono_literals;
using miral::WindowManagerTools;

namespace
{
std::string const a_window{"a window"};

struct WindowProperties;

struct WindowProperties : public miral::TestServer
{
    void SetUp() override
    {
        miral::TestServer::SetUp();
        client_connection = connect_client("WindowProperties");
        surface = Surface{mir_connection_create_render_surface_sync(client_connection, 40, 40)};
    }

    void TearDown() override
    {
        surface.reset();
        client_connection.reset();
        miral::TestServer::TearDown();
    }

    Connection client_connection;
    Surface surface;

    std::unique_ptr<TestWindowManagerPolicy> build_window_manager_policy(WindowManagerTools const& tools) override;

    void paint(Surface const& surface)
    {
        mir_buffer_stream_swap_buffers_sync(
            mir_render_surface_get_buffer_stream(surface, 50, 50, mir_pixel_format_argb_8888));
    }

    mir::test::Signal window_ready;
};

auto WindowProperties::build_window_manager_policy(WindowManagerTools const& tools)
-> std::unique_ptr<miral::TestServer::TestWindowManagerPolicy>
{
    struct MockWindowManagerPolicy : miral::TestServer::TestWindowManagerPolicy
    {
        using miral::TestServer::TestWindowManagerPolicy::TestWindowManagerPolicy;
        MOCK_METHOD1(advise_focus_gained, void (miral::WindowInfo const& window_info));
    };

    auto result = std::make_unique<MockWindowManagerPolicy>(tools, *this);

    ON_CALL(*result, advise_focus_gained(_))
        .WillByDefault(InvokeWithoutArgs([this] { window_ready.raise(); }));

    return result;
}
}

TEST_F(WindowProperties, on_creation_default_shell_chrome_is_normal)
{
    auto const window = WindowSpec::for_normal_window(client_connection, 50, 50)
        .set_name(a_window.c_str())
        .add_surface(surface, 50, 50, 0, 0)
        .create_window();

    paint(surface);
    ASSERT_TRUE(window_ready.wait_for(400ms));

    invoke_tools([&](WindowManagerTools& tools)
    {
        EXPECT_THAT(tools.info_for(tools.active_window()).shell_chrome(), Eq(mir_shell_chrome_normal));
    });
}

TEST_F(WindowProperties, on_creation_client_setting_shell_chrome_low_is_seen_by_window_manager)
{
    auto const window = WindowSpec::for_normal_window(client_connection, 50, 50)
        .set_name(a_window.c_str())
        .set_shell_chrome(mir_shell_chrome_low)
        .add_surface(surface, 50, 50, 0, 0)
        .create_window();

    paint(surface);
    ASSERT_TRUE(window_ready.wait_for(400ms));

    invoke_tools([&](WindowManagerTools& tools)
    {
        EXPECT_THAT(tools.info_for(tools.active_window()).shell_chrome(), Eq(mir_shell_chrome_low));
    });
}

TEST_F(WindowProperties, after_creation_client_setting_shell_chrome_low_is_seen_by_window_manager)
{
    auto const window = WindowSpec::for_normal_window(client_connection, 50, 50)
        .set_name(a_window.c_str())
        .add_surface(surface, 50, 50, 0, 0)
        .create_window();

    WindowSpec::for_changes(client_connection)
        .set_shell_chrome(mir_shell_chrome_low)
        .apply_to(window);

    paint(surface);

    ASSERT_TRUE(window_ready.wait_for(400ms));

    invoke_tools([&](WindowManagerTools& tools)
    {
        EXPECT_THAT(tools.info_for(tools.active_window()).shell_chrome(), Eq(mir_shell_chrome_low));
    });
}

TEST_F(WindowProperties, after_creation_client_setting_shell_chrome_normal_is_seen_by_window_manager)
{
    auto const window = WindowSpec::for_normal_window(client_connection, 50, 50)
        .set_name(a_window.c_str())
        .set_shell_chrome(mir_shell_chrome_low)
        .add_surface(surface, 50, 50, 0, 0)
        .create_window();

    WindowSpec::for_changes(client_connection)
        .set_shell_chrome(mir_shell_chrome_normal)
        .apply_to(window);

    paint(surface);
    ASSERT_TRUE(window_ready.wait_for(400ms));

    invoke_tools([&](WindowManagerTools& tools)
    {
        EXPECT_THAT(tools.info_for(tools.active_window()).shell_chrome(), Eq(mir_shell_chrome_normal));
    });
}
