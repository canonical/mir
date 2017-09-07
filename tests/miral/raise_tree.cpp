/*
 * Copyright © 2017 Canonical Ltd.
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
Width  const display_width{640};
Height const display_height{480};

Rectangle const display_area{{display_left, display_top}, {display_width, display_height}};

struct RaiseTree : TestWindowManagerTools
{
    Size const initial_parent_size{600, 400};
    Size const initial_child_size{300, 300};

    Window parent;
    Window child;
    Window another_window;

    void SetUp() override
    {
        basic_window_manager.add_display_for_testing(display_area);

        mir::scene::SurfaceCreationParameters creation_parameters;
        basic_window_manager.add_session(session);

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(Invoke([this](WindowInfo const& window_info){ parent = window_info.window(); }))
            .WillOnce(Invoke([this](WindowInfo const& window_info){ child = window_info.window(); }))
            .WillOnce(Invoke([this](WindowInfo const& window_info){ another_window = window_info.window(); }));

        creation_parameters.size = initial_parent_size;
        basic_window_manager.add_surface(session, creation_parameters, &create_surface);

        creation_parameters.type = mir_window_type_menu;
        creation_parameters.parent = parent;
        creation_parameters.size = initial_child_size;
        basic_window_manager.add_surface(session, creation_parameters, &create_surface);

        creation_parameters.type = mir_window_type_normal;
        creation_parameters.parent.reset();
        creation_parameters.size = display_area.size;
        basic_window_manager.add_surface(session, creation_parameters, &create_surface);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);
    }
};
}

TEST_F(RaiseTree, when_parent_is_raised_child_is_raised)
{
    EXPECT_CALL(*window_manager_policy, advise_raise(ElementsAre(parent, child)));
    basic_window_manager.raise_tree(parent);
}

TEST_F(RaiseTree, when_child_is_raised_parent_is_raised)
{
    EXPECT_CALL(*window_manager_policy, advise_raise(ElementsAre(parent, child)));
    EXPECT_CALL(*window_manager_policy, advise_raise(ElementsAre(child)));
    basic_window_manager.raise_tree(child);
}
