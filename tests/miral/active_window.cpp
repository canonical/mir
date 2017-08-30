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

#include "test_server.h"

#include <mir/client/surface.h>
#include <mir/client/window.h>
#include <mir/client/window_spec.h>
#include <mir_toolkit/mir_buffer_stream.h>

#include <miral/application_info.h>

#include <mir/test/signal.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace mir::client;
using namespace std::chrono_literals;
using miral::WindowManagerTools;


namespace
{
class FocusChangeSync
{
public:
    void exec(std::function<void()> const& f)
    {
        signal.reset();
        f();
        signal.wait_for(100ms);
    }

    static void raise_signal_on_focus_change(MirWindow* /*surface*/, MirEvent const* event, void* context)
    {
        if (mir_event_get_type(event) == mir_event_type_window &&
            mir_window_event_get_attribute(mir_event_get_window_event(event)) == mir_window_attrib_focus)
        {
            ((FocusChangeSync*)context)->signal.raise();
        }
    }

    auto signal_raised() -> bool { return signal.raised(); }

private:
    mir::test::Signal signal;
};

struct TestWindow : Surface, Window
{
    using Surface::operator=;
    using Window::operator=;
};

struct ActiveWindow : public miral::TestServer
{
    FocusChangeSync sync1;
    FocusChangeSync sync2;

    void paint(Surface const& surface)
    {
        mir_buffer_stream_swap_buffers_sync(
            mir_render_surface_get_buffer_stream(surface, 50, 50, mir_pixel_format_argb_8888));
    }

    auto create_window(Connection const& connection, char const* name, FocusChangeSync& sync) -> TestWindow
    {
        TestWindow result;

        result = Surface{mir_connection_create_render_surface_sync(connection, 50, 50)};

        auto const spec = WindowSpec::for_normal_window(connection, 50, 50)
            .set_event_handler(&FocusChangeSync::raise_signal_on_focus_change, &sync)
            .add_surface(result, 50, 50, 0, 0)
            .set_name(name);

        result = Window{spec.create_window()};

        sync.exec([&]{ paint(result); });

        EXPECT_TRUE(sync.signal_raised());

        return result;
    }

    auto create_tip(Connection const& connection, char const* name, Window const& parent, FocusChangeSync& sync) -> TestWindow
    {
        TestWindow result;
        result = Surface{mir_connection_create_render_surface_sync(connection, 50, 50)};

        MirRectangle aux_rect{10, 10, 10, 10};
        auto const spec = WindowSpec::for_tip(connection, 50, 50, parent, &aux_rect, mir_edge_attachment_any)
            .set_event_handler(&FocusChangeSync::raise_signal_on_focus_change, &sync)
            .add_surface(result, 50, 50, 0, 0)
            .set_name(name);

        result = Window{spec.create_window()};

        // Expect this to timeout: A tip should not receive focus
        sync.exec([&]{ paint(result); });
        EXPECT_FALSE(sync.signal_raised());

        return result;
    }

    auto create_dialog(Connection const& connection, char const* name, Window const& parent, FocusChangeSync& sync) -> TestWindow
    {
        TestWindow result;
        result = Surface{mir_connection_create_render_surface_sync(connection, 50, 50)};

        auto const spec = WindowSpec::for_dialog(connection, 50, 50, parent)
            .set_event_handler(&FocusChangeSync::raise_signal_on_focus_change, &sync)
            .add_surface(result, 50, 50, 0, 0)
            .set_name(name);

        result = Window{spec.create_window()};

        sync.exec([&]{ paint(result); });
        EXPECT_TRUE(sync.signal_raised());

        return result;
    }

    void assert_no_active_window()
    {
        invoke_tools([&](WindowManagerTools& tools)
            {
                auto const window = tools.active_window();
                ASSERT_FALSE(window);
            });
    }

    void assert_active_window_is(char const* const name)
    {
        invoke_tools([&](WindowManagerTools& tools)
            {
                 auto const window = tools.active_window();
                 ASSERT_TRUE(window);
                 auto const& window_info = tools.info_for(window);
                 ASSERT_THAT(window_info.name(), Eq(name));
            });
    }
};

auto const another_name = "second";
}

TEST_F(ActiveWindow, a_single_window_when_ready_becomes_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const window = create_window(connection, test_name, sync1);

    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, a_single_window_when_hiding_becomes_inactive)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);
    auto const window = create_window(connection, test_name, sync1);

    sync1.exec([&]{ mir_window_set_state(window, mir_window_state_hidden); });

    EXPECT_TRUE(sync1.signal_raised());
    assert_no_active_window();
}

TEST_F(ActiveWindow, a_single_window_when_unhiding_becomes_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);
    auto const window = create_window(connection, test_name, sync1);

    sync1.exec([&]{ mir_window_set_state(window, mir_window_state_hidden); });

    sync1.exec([&]{ mir_window_set_state(window, mir_window_state_restored); });

    EXPECT_TRUE(sync1.signal_raised());

    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, a_second_window_when_ready_becomes_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const first_window = create_window(connection, "first", sync1);
    auto const window = create_window(connection, test_name, sync2);

    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, a_second_window_hiding_makes_first_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const first_window = create_window(connection, test_name, sync1);
    auto const window = create_window(connection, another_name, sync2);

    sync2.exec([&]{ mir_window_set_state(window, mir_window_state_hidden); });

    EXPECT_TRUE(sync2.signal_raised());
    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, a_second_window_unhiding_leaves_first_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const first_window = create_window(connection, test_name, sync1);
    auto const window = create_window(connection, another_name, sync2);

    sync1.exec([&]{ mir_window_set_state(window, mir_window_state_hidden); });

    // Expect this to timeout
    sync2.exec([&]{ mir_window_set_state(window, mir_window_state_restored); });

    EXPECT_THAT(sync2.signal_raised(), Eq(false));
    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, switching_from_a_second_window_makes_first_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const first_window = create_window(connection, test_name, sync1);
    auto const window = create_window(connection, another_name, sync2);

    sync1.exec([&]{ invoke_tools([](WindowManagerTools& tools){ tools.focus_next_within_application(); }); });

    EXPECT_TRUE(sync1.signal_raised());
    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, switching_from_a_second_application_makes_first_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);
    auto const second_connection = connect_client(another_name);

    auto const first_window = create_window(connection, test_name, sync1);
    auto const window = create_window(second_connection, another_name, sync2);

    sync1.exec([&]{ invoke_tools([](WindowManagerTools& tools){ tools.focus_next_application(); }); });

    EXPECT_TRUE(sync1.signal_raised());
    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, closing_a_second_application_makes_first_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const first_window = create_window(connection, test_name, sync1);

    sync1.exec([&]
        {
            auto const second_connection = connect_client(another_name);
            auto const window = create_window(second_connection, another_name, sync2);
            assert_active_window_is(another_name);
        });

    EXPECT_TRUE(sync1.signal_raised());
    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, selecting_a_tip_makes_parent_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const parent = create_window(connection, test_name, sync1);

    miral::Window parent_window;
    invoke_tools([&](WindowManagerTools& tools){ parent_window = tools.active_window(); });

    // Steal the focus
    auto second_connection = connect_client(another_name);
    auto second_surface = create_window(second_connection, another_name, sync2);

    auto const tip = create_tip(connection, "tip", parent, sync2);

    sync1.exec([&]
        {
            invoke_tools([&](WindowManagerTools& tools)
                { tools.select_active_window(*tools.info_for(parent_window).children().begin()); });
        });
    EXPECT_TRUE(sync1.signal_raised());

    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, selecting_a_parent_makes_dialog_active)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const dialog_name = "dialog";
    auto const connection = connect_client(test_name);

    auto const parent = create_window(connection, test_name, sync1);

    miral::Window parent_window;
    invoke_tools([&](WindowManagerTools& tools){ parent_window = tools.active_window(); });

    auto const dialog = create_dialog(connection, dialog_name, parent, sync2);

    // Steal the focus
    auto second_connection = connect_client(another_name);
    auto second_surface = create_window(second_connection, another_name, sync1);

    sync2.exec([&]{ invoke_tools([&](WindowManagerTools& tools){ tools.select_active_window(parent_window); }); });

    EXPECT_TRUE(sync2.signal_raised());
    assert_active_window_is(dialog_name);
}

TEST_F(ActiveWindow, input_methods_are_not_focussed)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const parent = create_window(connection, test_name, sync1);
    auto const input_method = WindowSpec::for_input_method(connection, 50, 50, parent).create_window();

    assert_active_window_is(test_name);

    invoke_tools([&](WindowManagerTools& tools)
        {
            auto const& info = tools.info_for(tools.active_window());
            tools.select_active_window(info.children().at(0));
        });

    assert_active_window_is(test_name);
}

TEST_F(ActiveWindow, satellites_are_not_focussed)
{
    char const* const test_name = __PRETTY_FUNCTION__;
    auto const connection = connect_client(test_name);

    auto const parent = create_window(connection, test_name, sync1);
    auto const satellite = WindowSpec::for_satellite(connection, 50, 50, parent).create_window();

    assert_active_window_is(test_name);

    invoke_tools([&](WindowManagerTools& tools)
    {
        auto const& info = tools.info_for(tools.active_window());
        tools.select_active_window(info.children().at(0));
    });

    assert_active_window_is(test_name);
}

// lp:1671072
TEST_F(ActiveWindow, hiding_active_dialog_makes_parent_active)
{
    char const* const parent_name = __PRETTY_FUNCTION__;
    auto const dialog_name = "dialog";
    auto const connection = connect_client(parent_name);

    auto const parent = create_window(connection, parent_name, sync1);
    auto const dialog = create_dialog(connection, dialog_name, parent, sync2);

    sync1.exec([&]{ mir_window_set_state(dialog, mir_window_state_hidden); });

    EXPECT_TRUE(sync1.signal_raised());

    assert_active_window_is(parent_name);
}

TEST_F(ActiveWindow, when_another_window_is_about_hiding_active_dialog_makes_parent_active)
{
    FocusChangeSync sync3;
    char const* const parent_name = __PRETTY_FUNCTION__;
    auto const dialog_name = "dialog";
    auto const another_window_name = "another window";
    auto const connection = connect_client(parent_name);

    auto const parent = create_window(connection, parent_name, sync1);
    auto const another_window = create_window(connection, another_window_name, sync2);
    auto const dialog = create_dialog(connection, dialog_name, parent, sync3);

    sync1.exec([&]{ mir_window_set_state(dialog, mir_window_state_hidden); });

    EXPECT_TRUE(sync1.signal_raised());

    assert_active_window_is(parent_name);
}
