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
 */

#include "test_window_manager_tools.h"

using namespace miral;
using namespace testing;
namespace mt = mir::test;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;

namespace
{
Rectangle const display_area{{30,  40}, {1280, 720}};

struct WindowPlacementMaximized : mt::TestWindowManagerTools
{
    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    auto create_window(msh::SurfaceSpecification creation_parameters) -> Window
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

TEST_F(WindowPlacementMaximized, window_is_initially_placed_correctly)
{
    Window window;
    {
        msh::SurfaceSpecification params;
        params.state = mir_window_state_maximized;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    EXPECT_THAT(info.state(), Eq(mir_window_state_maximized));
    EXPECT_THAT(window.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(window.size(), Eq(display_area.size));
}

TEST_F(WindowPlacementMaximized, window_is_placed_correctly_when_maximized)
{
    Window window;
    {
        msh::SurfaceSpecification params;
        params.state = mir_window_state_restored;
        params.set_size({100, 100});
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    {
        msh::SurfaceSpecification spec;
        spec.state = mir_window_state_maximized;
        basic_window_manager.modify_surface(session, window, spec);
    }

    EXPECT_THAT(info.state(), Eq(mir_window_state_maximized));
    EXPECT_THAT(window.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(window.size(), Eq(display_area.size));
}

TEST_F(WindowPlacementMaximized, window_size_maintained_after_maximization_and_restoration)
{
    Size window_size{100, 200};

    Window window;
    {
        msh::SurfaceSpecification params;
        params.state = mir_window_state_restored;
        params.set_size(window_size);
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    {
        msh::SurfaceSpecification spec;
        spec.state = mir_window_state_maximized;
        basic_window_manager.modify_surface(session, window, spec);
    }

    {
        msh::SurfaceSpecification spec;
        spec.state = mir_window_state_restored;
        basic_window_manager.modify_surface(session, window, spec);
    }

    EXPECT_THAT(info.state(), Eq(mir_window_state_restored));
    EXPECT_THAT(window.size(), Eq(window_size));
}

TEST_F(WindowPlacementMaximized, initially_maximized_window_given_smaller_size_when_restored)
{
    Window window;
    {
        msh::SurfaceSpecification params;
        params.state = mir_window_state_maximized;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);
    auto const maximized_size = window.size();

    {
        msh::SurfaceSpecification spec;
        spec.state = mir_window_state_restored;
        basic_window_manager.modify_surface(session, window, spec);
    }

    EXPECT_THAT(info.state(), Eq(mir_window_state_restored));
    EXPECT_THAT(window.size().width, Lt(maximized_size.width));
    EXPECT_THAT(window.size().height, Lt(maximized_size.height));
}

TEST_F(WindowPlacementMaximized, window_can_be_maximized_onto_a_logical_output_group)
{
    Rectangle second_display_area{{1400, 70}, {640, 480}};
    Rectangle composit_display_area{Rectangles{display_area, second_display_area}.bounding_rectangle()};

    notify_configuration_applied(create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, second_display_area)}));

    Window window;
    {
        msh::SurfaceSpecification params;
        params.state = mir_window_state_restored;
        params.set_size({100, 100});
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    {
        msh::SurfaceSpecification spec;
        spec.state = mir_window_state_maximized;
        basic_window_manager.modify_surface(session, window, spec);
    }

    EXPECT_THAT(info.state(), Eq(mir_window_state_maximized));
    EXPECT_THAT(window.top_left(), Eq(composit_display_area.top_left));
    EXPECT_THAT(window.size(), Eq(composit_display_area.size));
}

TEST_F(WindowPlacementMaximized, maximized_window_is_placed_correctly_when_logical_output_groups_change)
{
    Rectangle second_display_area{{1400, 70}, {640, 480}};
    Rectangle composit_display_area{Rectangles{display_area, second_display_area}.bounding_rectangle()};

    notify_configuration_applied(create_fake_display_configuration({display_area, second_display_area}));

    Window window;
    {
        msh::SurfaceSpecification params;
        params.state = mir_window_state_maximized;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    notify_configuration_applied(create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, second_display_area)}));

    EXPECT_THAT(info.state(), Eq(mir_window_state_maximized));
    EXPECT_THAT(window.top_left(), Eq(composit_display_area.top_left));
    EXPECT_THAT(window.size(), Eq(composit_display_area.size));
}
