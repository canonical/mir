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

struct ModifyWindowState : mt::TestWindowManagerTools, WithParamInterface<MirWindowType>
{
    Size const initial_parent_size{600, 400};

    mir::scene::SurfaceCreationParameters creation_parameters;
    Window window;

    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
        creation_parameters.type = mir_window_type_normal;
        creation_parameters.size = initial_parent_size;
    }

    void create_window()
    {
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

TEST_P(ModifyWindowState, modify_state)
{
    auto const original_state = mir_window_state_restored;
    auto const new_state = MirWindowState(GetParam());
    auto const state_is_visible = (new_state != mir_window_state_minimized) && (new_state != mir_window_state_hidden);

    creation_parameters.state = original_state;
    create_window();
    auto const& info = window_manager_tools.info_for(window);

    WindowSpecification mods;
    mods.state() = new_state;
    window_manager_tools.modify_window(window, mods);
    EXPECT_THAT(std::shared_ptr<mir::scene::Surface>(window)->state(), Eq(new_state));
    EXPECT_THAT(info.state(), Eq(new_state));
    EXPECT_THAT(info.is_visible(), Eq(state_is_visible)) << "State is " << new_state;

    mods.state() = original_state;
    window_manager_tools.modify_window(window, mods);
    EXPECT_THAT(std::shared_ptr<mir::scene::Surface>(window)->state(), Eq(original_state));
    EXPECT_THAT(info.state(), Eq(original_state));
    EXPECT_TRUE(info.is_visible());
}

TEST_P(ModifyWindowState, initial_client_facing_state)
{
    auto const miral_state = mir_window_state_maximized;
    auto const client_facing_state = MirWindowState(GetParam());

    creation_parameters.state = miral_state;
    ON_CALL(*window_manager_policy, place_new_window(_, _))
        .WillByDefault([&](auto, auto spec)
            {
                spec.client_facing_state() = client_facing_state;
                return spec;
            });
    create_window();
    auto const& info = window_manager_tools.info_for(window);

    EXPECT_THAT(std::shared_ptr<mir::scene::Surface>(window)->state(), Eq(client_facing_state));
    EXPECT_THAT(info.state(), Eq(miral_state));
}

TEST_P(ModifyWindowState, modify_client_facing_state)
{
    auto const miral_state = mir_window_state_maximized;
    auto const client_facing_state = MirWindowState(GetParam());

    creation_parameters.state = miral_state;
    create_window();
    auto const& info = window_manager_tools.info_for(window);

    WindowSpecification mods;
    mods.client_facing_state() = client_facing_state;
    window_manager_tools.modify_window(window, mods);

    EXPECT_THAT(std::shared_ptr<mir::scene::Surface>(window)->state(), Eq(client_facing_state));
    EXPECT_THAT(info.state(), Eq(miral_state));
}
}

INSTANTIATE_TEST_SUITE_P(ModifyWindowState, ModifyWindowState, ::testing::Values(
//    mir_window_state_unknown,
    mir_window_state_restored,
    mir_window_state_minimized,
    mir_window_state_maximized,
    mir_window_state_vertmaximized,
    mir_window_state_fullscreen,
    mir_window_state_horizmaximized,
    mir_window_state_hidden
//    mir_window_states
));
