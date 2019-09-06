/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "test_window_manager_tools.h"

using namespace miral;
using namespace testing;
namespace mt = mir::test;
namespace mg = mir::graphics;

namespace
{
X const display_left{30};
Y const display_top{40};
Width const display_width{1280};
Height const display_height{720};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

struct WindowPlacementFullscreen : mt::TestWindowManagerTools
{
    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    auto create_window(mir::scene::SurfaceCreationParameters creation_parameters) -> Window
    {
        Window result;

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [&result](WindowInfo const& window_info)
                        { result = window_info.window(); }));

        basic_window_manager.add_surface(session, creation_parameters, &create_surface);
        basic_window_manager.select_active_window(result);

        // Clear the expectations used to capture the window
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return result;
    }
};

}

TEST_F(WindowPlacementFullscreen, window_is_initially_placed_correctly)
{
    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_fullscreen;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    EXPECT_THAT(info.state(), Eq(mir_window_state_fullscreen));
    EXPECT_THAT(window.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(window.size(), Eq(display_area.size));
}

TEST_F(WindowPlacementFullscreen, window_is_placed_correctly_when_fullscreened)
{
    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    {
        WindowSpecification spec;
        spec.state() = mir_window_state_fullscreen;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(info.state(), Eq(mir_window_state_fullscreen));
    EXPECT_THAT(window.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(window.size(), Eq(display_area.size));
}

TEST_F(WindowPlacementFullscreen, window_can_be_unfullscreened)
{
    Size window_size{100, 200};

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_restored;
        params.size = window_size;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    {
        WindowSpecification spec;
        spec.state() = mir_window_state_fullscreen;
        window_manager_tools.modify_window(window, spec);
    }

    {
        WindowSpecification spec;
        spec.state() = mir_window_state_restored;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(info.state(), Eq(mir_window_state_restored));
    EXPECT_THAT(window.size(), Eq(window_size));
}

TEST_F(WindowPlacementFullscreen, window_initially_placed_correctly_when_output_id_set)
{
    Rectangle const other_display_area = {{1400, 10}, {960, 720}};
    mg::DisplayConfigurationOutputId output_id{2};

    auto const display_config = create_fake_display_configuration({display_area, other_display_area});
    notify_configuration_applied(display_config);

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_fullscreen;
        params.output_id = output_id;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    EXPECT_THAT(info.state(), Eq(mir_window_state_fullscreen));
    EXPECT_THAT(window.top_left(), Eq(other_display_area.top_left));
    EXPECT_THAT(window.size(), Eq(other_display_area.size));
}

TEST_F(WindowPlacementFullscreen, window_placed_correctly_when_output_id_changes)
{
    Rectangle const other_display_area = {{1400, 10}, {960, 720}};
    mg::DisplayConfigurationOutputId output_id_a{1};
    mg::DisplayConfigurationOutputId output_id_b{2};

    auto const display_config = create_fake_display_configuration({display_area, other_display_area});
    notify_configuration_applied(display_config);

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_fullscreen;
        params.output_id = output_id_b;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    {
        WindowSpecification spec;
        spec.output_id() = output_id_a.as_value();
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(info.state(), Eq(mir_window_state_fullscreen));
    EXPECT_THAT(window.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(window.size(), Eq(display_area.size));
}
