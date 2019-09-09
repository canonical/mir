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

#include "test_window_manager_tools.h"
#include <mir/event_printer.h>

using namespace miral;
using namespace testing;
namespace mt = mir::test;
using mir::operator<<;

namespace
{
X const display_left{0};
Y const display_top{0};
Width const display_width{640};
Height const display_height{480};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

struct DisplayConfiguration : mt::TestWindowManagerTools
{
    Size const initial_window_size{600, 400};

    Window window;

    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    void create_fullscreen_window()
    {
        mir::scene::SurfaceCreationParameters creation_parameters;
        creation_parameters.type = mir_window_type_normal;
        creation_parameters.size = initial_window_size;
        creation_parameters.state = mir_window_state_fullscreen;
        creation_parameters.output_id = mir::graphics::DisplayConfigurationOutputId{0};

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [this](WindowInfo const& window_info)
                        { window = window_info.window(); }));

        basic_window_manager.add_surface(session, creation_parameters, &create_surface);
        basic_window_manager.select_active_window(window);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);
    }
};
}

// This is the scenario behind lp:1640557
TEST_F(DisplayConfiguration, given_fullscreen_windows_reconfiguring_displays_doesnt_crash)
{
    create_fullscreen_window();

    WindowSpecification mods;
    mods.state() = mir_window_state_hidden;
    window_manager_tools.modify_window(window, mods);
    mods.state() = mir_window_state_fullscreen;
    window_manager_tools.modify_window(window, mods);

    Rectangle const new_display{
        display_area.top_left + Displacement{as_delta(display_width), 0}, display_area.size};

    notify_configuration_applied(create_fake_display_configuration({display_area}));
    basic_window_manager.remove_display(new_display);
}
