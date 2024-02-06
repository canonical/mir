/*
 * Copyright © Canonical Ltd.
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
 */

#include "application_selector.h"
#include "test_window_manager_tools.h"
#include <mir/scene/session.h>

using namespace miral;
namespace mt = mir::test;
using namespace testing;

struct ApplicationSelectorTest : mt::TestWindowManagerTools
{
    ApplicationSelector application_selector;

    ApplicationSelectorTest()
        : mt::TestWindowManagerTools(),
          application_selector(window_manager_tools)
    {}

    auto create_window() -> Window
    {
        mir::shell::SurfaceSpecification creation_parameters;
        creation_parameters.type = mir_window_type_normal;
        creation_parameters.focus_mode = MirFocusMode::mir_focus_mode_focusable;
        creation_parameters.set_size({600, 400});
        auto window = create_and_select_window(creation_parameters);

        // Simulates a new application opening and gaining focus
        application_selector.advise_new_window(window_manager_tools.info_for(window));
        application_selector.advise_focus_gained(window_manager_tools.info_for(window));
        return window;
    }

    auto create_window_list(int num_windows) -> std::vector<Window>
    {
        std::vector<Window> window_list;
        for (int index = 0; index < num_windows; index++)
        {
            window_list.push_back(create_window());
        }

        return window_list;
    }
};

TEST_F(ApplicationSelectorTest, focused_window_is_latest_selected)
{
    auto windows = create_window_list(3);
    EXPECT_TRUE(windows[2] == application_selector.get_focused());
}

TEST_F(ApplicationSelectorTest, moving_forward_once_selects_previously_selected_window)
{
    auto windows = create_window_list(3);
    auto window = application_selector.next(false);
    EXPECT_TRUE(window == windows[1]);
}

TEST_F(ApplicationSelectorTest, moving_backward_once_selects_least_recently_selected_window)
{
    auto windows = create_window_list(3);
    auto window = application_selector.prev(false);
    EXPECT_TRUE(window == windows[0]);
}

TEST_F(ApplicationSelectorTest, can_move_forward_through_all_windows_in_the_list_to_arrive_at_original_window)
{
    int num_windows = 3;
    auto windows = create_window_list(num_windows);
    for (int i = 0; i < 3; i++)
        application_selector.next(false);
    auto window = application_selector.complete();
    EXPECT_TRUE(window == windows[2]);
}

TEST_F(ApplicationSelectorTest, new_selector_is_not_active)
{
    auto windows = create_window_list(3);
    EXPECT_FALSE(application_selector.is_active());
}

TEST_F(ApplicationSelectorTest, calling_next_with_no_windows_results_in_default_window)
{
    auto window = application_selector.next(false);
    EXPECT_TRUE(window == Window());
    EXPECT_FALSE(application_selector.is_active());
}

TEST_F(ApplicationSelectorTest, focusing_a_new_window_while_application_selector_is_active_focuses_the_new_window)
{
    auto windows = create_window_list(3);
    application_selector.prev(false);
    auto new_window = create_window();
    auto focused = application_selector.get_focused();
    EXPECT_TRUE(focused == new_window);
}

TEST_F(ApplicationSelectorTest, window_added_while_selection_is_active_get_added_to_end_of_list)
{
    auto windows = create_window_list(3);
    application_selector.next(false);
    auto new_window = create_window();
    auto window = application_selector.prev(false);
    EXPECT_TRUE(window == windows[0]);
}

TEST_F(ApplicationSelectorTest, deleting_selected_window_makes_the_next_window_selected)
{
    auto windows = create_window_list(3);
    application_selector.next(false); // windows[1] is selected
    application_selector.advise_focus_gained(window_manager_tools.info_for(windows[1]));
    application_selector.advise_delete_window(window_manager_tools.info_for(windows[1]));
    auto window = application_selector.get_focused();
    EXPECT_EQ(window, windows[0]);
}

TEST_F(ApplicationSelectorTest, selecting_the_next_window_in_an_app_when_there_isnt_another_window_results_in_the_currently_selected_window_remaining_the_same)
{
    auto windows = create_window_list(3);
    application_selector.next(true); // windows[1] is selected
    auto window = application_selector.get_focused();
    EXPECT_TRUE(window == windows[2]);
}
