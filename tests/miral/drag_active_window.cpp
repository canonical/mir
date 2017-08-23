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

struct DragActiveWindow : TestWindowManagerTools, WithParamInterface<MirWindowType>
{
    Size const initial_parent_size{600, 400};

    Window window;

    void SetUp() override
    {
        basic_window_manager.add_display_for_testing(display_area);
        basic_window_manager.add_session(session);
    }

    void create_window_of_type(MirWindowType type)
    {
        mir::scene::SurfaceCreationParameters creation_parameters;
        creation_parameters.type = type;
        creation_parameters.size = initial_parent_size;

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

using ForMoveableTypes = DragActiveWindow;
using ForUnmoveableTypes = DragActiveWindow;

TEST_P(ForMoveableTypes, moves)
{
    create_window_of_type(GetParam());

    Displacement const movement{10, 10};
    auto const initial_position = window.top_left();
    auto const expected_position = initial_position + movement;

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));

    window_manager_tools.drag_active_window(movement);

    EXPECT_THAT(window.top_left(), Eq(expected_position))
                << "Type: " << GetParam();
}

TEST_P(ForUnmoveableTypes, doesnt_move)
{
    create_window_of_type(GetParam());

    Displacement const movement{10, 10};
    auto const expected_position = window.top_left();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, _)).Times(0);

    window_manager_tools.drag_active_window(movement);

    EXPECT_THAT(window.top_left(), Eq(expected_position))
                << "Type: " << GetParam();
}
}

// When a surface is moved interactively
// -------------------------------------
// Regular, floating regular, dialog, and satellite surfaces should be user-movable.
// Popups, glosses, and tips should not be.
// Freestyle surfaces may or may not be, as specified by the app.
//                              Mir and Unity: Surfaces, input, and displays (v0.3)
INSTANTIATE_TEST_CASE_P(DragActiveWindow, ForMoveableTypes, ::testing::Values(
    mir_window_type_normal,
    mir_window_type_utility,
    mir_window_type_dialog,
//    mir_window_type_gloss,
    mir_window_type_freestyle
//    mir_window_type_menu,
//    mir_window_type_inputmethod,
//    mir_window_type_satellite,
//    mir_window_type_tip,
//    mir_window_types
));


INSTANTIATE_TEST_CASE_P(DragActiveWindow, ForUnmoveableTypes, ::testing::Values(
//    mir_window_type_normal,
//    mir_window_type_utility,
//    mir_window_type_dialog,
    mir_window_type_gloss,
//    mir_window_type_freestyle,
    mir_window_type_menu,
    mir_window_type_inputmethod,
//    mir_window_type_satellite,
    mir_window_type_tip
//    mir_window_types
));

using DragWindow = DragActiveWindow;

TEST_F(DragWindow, can_drag_satellite)
{
    create_window_of_type(mir_window_type_satellite);

    Displacement const movement{10, 10};
    auto const initial_position = window.top_left();
    auto const expected_position = initial_position + movement;

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));

    window_manager_tools.drag_window(window, movement);

    EXPECT_THAT(window.top_left(), Eq(expected_position))
                << "Type: " << GetParam();
}