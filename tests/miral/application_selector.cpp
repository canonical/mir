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
};

/// Testing if we can cycle through 3 windows and then select one
/// with a "forward" selection strategy.
TEST_F(ApplicationSelectorTest, run_forward)
{
    // Create three windows on our display
    mir::shell::SurfaceSpecification creation_parameters;
    creation_parameters.name = "window1";
    creation_parameters.type = mir_window_type_normal;
    creation_parameters.focus_mode = MirFocusMode::mir_focus_mode_focusable;
    creation_parameters.set_size({600, 400});
    auto window1 = create_and_select_window(creation_parameters);
    creation_parameters.name = "window2";
    auto window2 = create_and_select_window(creation_parameters);
    creation_parameters.name = "window3";
    auto window3 = create_and_select_window(creation_parameters);

    // Make sure that window3 (the last one added) is the active one
    EXPECT_TRUE(window3 == basic_window_manager.active_window());

    // Start the selector and assert that the raised application is window1
    auto application = application_selector.next(false);
    EXPECT_TRUE(application == window1.application());

    // Call next and assert that we have moved to window2
    application = application_selector.next(false);
    EXPECT_TRUE(application == window2.application());

    // Stop the selector and assert that window2 is selected
    application = application_selector.next(false);
    EXPECT_TRUE(application == window2.application());
}

/// Testing if we can cycle through 3 windows and then select one
/// with a "reverse" selection strategy.
TEST_F(ApplicationSelectorTest, run_backward)
{
    // Create three windows on our display
    mir::shell::SurfaceSpecification creation_parameters;
    creation_parameters.name = "window1";
    creation_parameters.type = mir_window_type_normal;
    creation_parameters.focus_mode = MirFocusMode::mir_focus_mode_focusable;
    creation_parameters.set_size({600, 400});
    auto window1 = create_and_select_window(creation_parameters);
    creation_parameters.name = "window2";
    auto window2 = create_and_select_window(creation_parameters);
    creation_parameters.name = "window3";
    auto window3 = create_and_select_window(creation_parameters);

    // Make sure that window3 (the last one added) is the active one
    EXPECT_TRUE(window3 == basic_window_manager.active_window());

    // Start the selector and assert that the raised application is window2
    auto application = application_selector.next(true);
    EXPECT_TRUE(application == window2.application());

    // Call next and assert that we have moved to window1
    application = application_selector.next(true);
    EXPECT_TRUE(application == window1.application());

    // Stop the selector and assert that window1 is selected
    application = application_selector.complete();
    EXPECT_TRUE(application == window1.application());
}

/// Testing if we can start the selector when there are no sessions started.
TEST_F(ApplicationSelectorTest, run_with_no_sessions)
{
    // Start the selector and assert that an application could not be raised
    auto application = application_selector.next(false);
    EXPECT_TRUE(application == nullptr);
    EXPECT_FALSE(application_selector.is_active());
}

/// Testing if we can cycle through a single window successfully
TEST_F(ApplicationSelectorTest, run_in_circle)
{
    // Create three windows on our display
    mir::shell::SurfaceSpecification creation_parameters;
    creation_parameters.name = "window1";
    creation_parameters.type = mir_window_type_normal;
    creation_parameters.focus_mode = MirFocusMode::mir_focus_mode_focusable;
    creation_parameters.set_size({600, 400});
    auto window1 = create_and_select_window(creation_parameters);

    // Make sure that window3 (the last one added) is the active one
    EXPECT_TRUE(window1 == basic_window_manager.active_window());

    // Start the selector and assert that the raised application is window2
    auto application = application_selector.next(true);
    EXPECT_TRUE(application == window1.application());

    // Call next and assert that we have moved to window1
    application = application_selector.next(true);
    EXPECT_TRUE(application == window1.application());

    // Stop the selector and assert that window1 is selected
    application = application_selector.complete();
    EXPECT_TRUE(application == window1.application());
}