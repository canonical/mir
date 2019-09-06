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
auto const display_area = Rectangle{{0, 0}, {800, 600}};
auto const parent_width = 400;
auto const parent_height = 300;

auto placement(
    Rectangle const& aux_rect,
    MirPlacementGravity aux_rect_placement_gravity,
    MirPlacementGravity window_placement_gravity,
    MirPlacementHints placement_hints) -> WindowSpecification
{
    WindowSpecification modification;

    modification.aux_rect() = aux_rect;
    modification.aux_rect_placement_gravity() = aux_rect_placement_gravity;
    modification.window_placement_gravity() = window_placement_gravity;
    modification.placement_hints() = placement_hints;

    return modification;
}

struct WindowPlacementAnchorsToParent : mt::TestWindowManagerTools
{
    Size const parent_size{parent_width, parent_height};
    Size const initial_child_size{100, 50};

    Window parent;
    Window child;

    Point parent_position;
    WindowSpecification modification;

    void SetUp() override
    {
        TestWindowManagerTools::SetUp();

        notify_configuration_applied(create_fake_display_configuration({display_area}));

        mir::scene::SurfaceCreationParameters creation_parameters;
        basic_window_manager.add_session(session);

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(Invoke([this](WindowInfo const& window_info){ parent = window_info.window(); }))
            .WillOnce(Invoke([this](WindowInfo const& window_info){ child = window_info.window(); }));

        creation_parameters.size = parent_size;
        basic_window_manager.add_surface(session, creation_parameters, &create_surface);

        creation_parameters.type = mir_window_type_tip;
        creation_parameters.parent = parent;
        creation_parameters.size = initial_child_size;
        basic_window_manager.add_surface(session, creation_parameters, &create_surface);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        parent_position = parent.top_left();
    }
};
}

// there was an IRC conversation to sort this out between myself William and Thomas.
// I think the resulting consensus was:
//
//   1. Mir will constrain the placement anchor of the aux_rect to the parent
//      surface. I don't think we agreed exactly how (e.g. do we "clip" the
//      rect? What happens if there is *no* intersection?)
//
//   2. Mir will constrain the the offset placement anchor to the parent surface.
//      Again I don't think we agreed how. (Slide it horizontally and/or vertically
//      the minimum amount?)
//                                      - alan_g (mir-devel, Mon, 5 Sep 2016 17:21:01 +0100)
//
// What we have implemented is to constrain the result of offsetting to the parent. That
// seems to provide reasonable behaviour. Are there test cases that require something more?

TEST_F(WindowPlacementAnchorsToParent, given_rect_anchor_right_of_parent_client_is_anchored_to_parent)
{
    auto const rect_size = 10;
    Rectangle const overlapping_right{{parent_width-rect_size/2, parent_height/2}, {rect_size, rect_size}};

    modification = placement(
        overlapping_right,
        mir_placement_gravity_northeast,
        mir_placement_gravity_northwest,
        MirPlacementHints(mir_placement_hints_slide_y|mir_placement_hints_resize_x));

    auto const expected_position = parent_position + Displacement{parent_width, parent_height/2};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, _)).Times(0);
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(initial_child_size));
}

TEST_F(WindowPlacementAnchorsToParent, given_rect_anchor_above_parent_client_is_anchored_to_parent)
{
    auto const rect_size = 10;
    Rectangle const overlapping_above{{parent_width/2, -rect_size/2}, {rect_size, rect_size}};

    modification = placement(
        overlapping_above,
        mir_placement_gravity_northeast,
        mir_placement_gravity_southeast,
        mir_placement_hints_slide_x);

    auto const expected_position = parent_position + DeltaX{parent_width/2 + rect_size}
                                   - as_displacement(initial_child_size);

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, _)).Times(0);
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(initial_child_size));
}

TEST_F(WindowPlacementAnchorsToParent, given_offset_right_of_parent_client_is_anchored_to_parent)
{
    auto const rect_size = 10;
    Rectangle const mid_right{{parent_width-rect_size, parent_height/2}, {rect_size, rect_size}};

    modification = placement(
        mid_right,
        mir_placement_gravity_northeast,
        mir_placement_gravity_northwest,
        MirPlacementHints(mir_placement_hints_slide_y|mir_placement_hints_resize_x));

    modification.aux_rect_placement_offset() = Displacement{rect_size, 0};

    auto const expected_position = parent_position + Displacement{parent_width, parent_height/2};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, _)).Times(0);
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(initial_child_size));
}

TEST_F(WindowPlacementAnchorsToParent, given_offset_above_parent_client_is_anchored_to_parent)
{
    auto const rect_size = 10;
    Rectangle const mid_top{{parent_width/2, 0}, {rect_size, rect_size}};

    modification = placement(
        mid_top,
        mir_placement_gravity_northeast,
        mir_placement_gravity_southeast,
        mir_placement_hints_slide_x);

    modification.aux_rect_placement_offset() = Displacement{0, -rect_size};

    auto const expected_position = parent_position + DeltaX{parent_width/2 + rect_size}
                                   - as_displacement(initial_child_size);

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, _)).Times(0);
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(initial_child_size));
}

TEST_F(WindowPlacementAnchorsToParent, given_rect_and_offset_below_left_parent_client_is_anchored_to_parent)
{
    auto const rect_size = 10;
    Rectangle const below_left{{-rect_size, parent_height}, {rect_size, rect_size}};

    modification = placement(
        below_left,
        mir_placement_gravity_southwest,
        mir_placement_gravity_northeast,
        mir_placement_hints_resize_any);

    modification.aux_rect_placement_offset() = Displacement{-rect_size, rect_size};

    auto const expected_position = parent_position + DeltaY{parent_height} - as_displacement(initial_child_size).dx;

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, _)).Times(0);
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(initial_child_size));
}
