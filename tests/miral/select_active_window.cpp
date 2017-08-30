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

using namespace miral;
using namespace testing;
namespace mt = mir::test;

namespace
{
X const display_left{0};
Y const display_top{0};
Width const display_width{1280};
Height const display_height{720};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

struct SelectActiveWindow : TestWindowManagerTools
{

    void SetUp() override
    {
        basic_window_manager.add_display_for_testing(display_area);
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

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return result;
    }
};
}

namespace std
{
auto operator<<(std::ostream& out, miral::Window const& window) -> std::ostream&
{
    if (std::shared_ptr<mir::scene::Surface> surface = window)
        return out << surface->name();
    else
        return out << "(nul)";
}
}

// lp:1626659
// "If the surface has a child dialog, the deepest descendant
// dialog should receive input focus."
TEST_F(SelectActiveWindow, given_a_child_dialog_when_selecting_the_parent_the_dialog_receives_focus)
{
    mir::scene::SurfaceCreationParameters creation_parameters;
    creation_parameters.name = "parent";
    creation_parameters.type = mir_window_type_normal;
    creation_parameters.size = Size{600, 400};

    auto parent = create_window(creation_parameters);

    creation_parameters.name = "dialog";
    creation_parameters.type = mir_window_type_dialog;
    creation_parameters.parent = parent;

    auto dialog = create_window(creation_parameters);

    auto actual = basic_window_manager.select_active_window(parent);
    EXPECT_THAT(actual, Eq(dialog))
        << "actual=" << actual << ", expected=" << dialog;
}

TEST_F(SelectActiveWindow, given_a_hidden_child_dialog_when_selecting_the_parent_the_parent_receives_focus)
{
    mir::scene::SurfaceCreationParameters creation_parameters;
    creation_parameters.name = "parent";
    creation_parameters.type = mir_window_type_normal;
    creation_parameters.size = Size{600, 400};

    auto parent = create_window(creation_parameters);

    creation_parameters.name = "dialog";
    creation_parameters.type = mir_window_type_dialog;
    creation_parameters.parent = parent;

    auto dialog = create_window(creation_parameters);

    WindowSpecification mods;
    mods.state() = mir_window_state_hidden;
    basic_window_manager.modify_window(basic_window_manager.info_for(dialog), mods);

    auto actual = basic_window_manager.select_active_window(parent);
    EXPECT_THAT(actual, Eq(parent))
                << "actual=" << actual << ", expected=" << parent;
}
