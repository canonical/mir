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

namespace
{
Rectangle const display_area{{0, 0}, {1280, 720}};

struct InitialWindowPlacement : mt::TestWindowManagerTools
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

/// Regression test for https://github.com/MirServer/mir/issues/899
TEST_F(InitialWindowPlacement, window_is_not_placed_off_screen_when_existing_window_is_close_to_edge)
{
    Size first_size{40, 35};
    Point first_top_left = display_area.bottom_right() - as_displacement(first_size);
    Size second_size{200, 300};

    Window first;
    {
        // Setting the top left in the initial params wasn't working, so we first create the window then move it
        mir::scene::SurfaceCreationParameters params;
        params.size = first_size;
        first = create_window(params);
        miral::WindowSpecification spec;
        spec.top_left() = first_top_left;
        basic_window_manager.modify_window(basic_window_manager.info_for(first), spec);
    }

    ASSERT_THAT(first.top_left(), Eq(first_top_left))
        << "Could not run test as the first window could not be positioned correctly";

    Window second;
    {
        mir::scene::SurfaceCreationParameters params;
        params.size = second_size;
        second = create_window(params);
    }
    Rectangle second_area{second.top_left(), second.size()};

    EXPECT_THAT(second_area.left(), Ge(display_area.left()));
    EXPECT_THAT(second_area.top(), Ge(display_area.top()));
    EXPECT_THAT(second_area.right(), Le(display_area.right()));
    EXPECT_THAT(second_area.bottom(), Le(display_area.bottom()));
}

TEST_F(InitialWindowPlacement, initially_maximized_window_covers_output)
{
    Window window;
    {
        // Setting the top left in the initial params wasn't working, so we first create the window then move it
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        window = create_window(params);
    }

    ASSERT_THAT(window.top_left(), Eq(display_area.top_left));
    ASSERT_THAT(window.size(), Eq(display_area.size));
}

TEST_F(InitialWindowPlacement, initially_maximized_window_covers_only_one_output)
{
    auto display_config = create_fake_display_configuration({
        display_area,
        {display_area.top_right(), display_area.size}});
    notify_configuration_applied(display_config);

    Window window;
    {
        // Setting the top left in the initial params wasn't working, so we first create the window then move it
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        window = create_window(params);
    }

    ASSERT_THAT(window.top_left(), Eq(display_area.top_left));
    ASSERT_THAT(window.size(), Eq(display_area.size));
}

