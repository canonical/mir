/*
 * Copyright © 2020 Canonical Ltd.
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
using namespace mir::graphics;
using namespace testing;
namespace mt = mir::test;
namespace ms = mir::scene;
namespace msh = mir::shell;

namespace
{
std::vector<Rectangle> const display = {{{20,  30}, {600, 400}}, {{620, 0},  {800, 500}}};

struct WindowPlacementOutput : mt::TestWindowManagerTools
{
    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration(display));
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

struct WhenOutputIsSpecified : WindowPlacementOutput, WithParamInterface<MirWindowState> {};
}

TEST_P(WhenOutputIsSpecified, window_is_initially_placed_on_it)
{
    Window window;
    {
        msh::SurfaceSpecification params;
        params.state = GetParam();
        params.output_id = DisplayConfigurationOutputId{2};
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    EXPECT_THAT(info.state(), Eq(GetParam()));
    EXPECT_TRUE(display[1].contains(Rectangle{window.top_left(), window.size()}))
        << "display=" << display[1] <<", window=" << Rectangle{window.top_left(), window.size()};
}

TEST_P(WhenOutputIsSpecified, window_is_moved_to_it)
{
    Window window;
    {
        msh::SurfaceSpecification params;
        params.state = mir_window_state_minimized;
        window = create_window(params);
    }

    {
        WindowSpecification modifications;
        modifications.state() = GetParam();
        modifications.output_id() = 2;

        auto& info = basic_window_manager.info_for(window);
        basic_window_manager.place_and_size_for_state(modifications, info);
        basic_window_manager.modify_window(info, modifications);
    }

    auto const& info = basic_window_manager.info_for(window);
    EXPECT_THAT(info.state(), Eq(GetParam()));
    EXPECT_TRUE(display[1].contains(Rectangle{window.top_left(), window.size()}))
        << "display=" << display[1] <<", window=" << Rectangle{window.top_left(), window.size()};
}

INSTANTIATE_TEST_SUITE_P(WindowPlacementOutput, WhenOutputIsSpecified, ::testing::Values(
    mir_window_state_maximized,
    mir_window_state_fullscreen
// These don't work with the existing place_and_size_for_state code (which uses the "restore_rect"
// and ignores the output). But maybe they should work if the output_id is supplied?
//    mir_window_state_restored,
//    mir_window_state_vertmaximized,
//    mir_window_state_horizmaximized,
));
