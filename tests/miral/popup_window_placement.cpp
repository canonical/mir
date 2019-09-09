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
Width  const display_width{640};
Height const display_height{480};

Rectangle const display_area{{display_left, display_top}, {display_width, display_height}};

auto const null_window = Window{};

mir::shell::SurfaceSpecification edge_attachment(Rectangle const& aux_rect, MirEdgeAttachment attachment)
{
    mir::shell::SurfaceSpecification result;
    result.aux_rect = aux_rect;
    result.edge_attachment = attachment;
    return result;
}

struct PopupWindowPlacement : mt::TestWindowManagerTools
{
    Size const initial_parent_size{600, 400};
    Size const initial_child_size{300, 300};
    Rectangle const rectangle_away_from_rhs{{20, 20}, {20, 20}};
    Rectangle const rectangle_near_rhs{{590, 20}, {10, 20}};
    Rectangle const rectangle_away_from_bottom{{20, 20}, {20, 20}};
    Rectangle const rectangle_near_bottom{{20, 380}, {20, 20}};
    Rectangle const rectangle_near_both_sides{{0, 20}, {600, 20}};
    Rectangle const rectangle_near_both_sides_and_bottom{{0, 380}, {600, 20}};
    Rectangle const rectangle_near_all_sides{{0, 20}, {600, 380}};
    Rectangle const rectangle_near_both_bottom_right{{400, 380}, {200, 20}};

    Window parent;
    Window child;

    WindowSpecification modification;

    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));

        mir::scene::SurfaceCreationParameters creation_parameters;
        basic_window_manager.add_session(session);

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(Invoke([this](WindowInfo const& window_info){ parent = window_info.window(); }))
            .WillOnce(Invoke([this](WindowInfo const& window_info){ child = window_info.window(); }));

        creation_parameters.size = initial_parent_size;
        basic_window_manager.add_surface(session, creation_parameters, &create_surface);

        creation_parameters.type = mir_window_type_menu;
        creation_parameters.parent = parent;
        creation_parameters.size = initial_child_size;
        basic_window_manager.add_surface(session, creation_parameters, &create_surface);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);
    }

    void TearDown() override
    {
//        std::cerr << "DEBUG parent position:" << Rectangle{parent.top_left(), parent.size()} << '\n';
//        std::cerr << "DEBUG child position :" << Rectangle{child.top_left(), child.size()} << '\n';
    }

    auto aux_rect_position() -> Rectangle
    {
        auto const rectangle = modification.aux_rect().value();
        return {rectangle.top_left + (parent.top_left() - Point{}), rectangle.size};
    }

    auto on_top_edge() -> Point
    {
        return aux_rect_position().top_left - as_displacement(child.size()).dy;
    }

    auto on_right_edge() -> Point
    {
        return aux_rect_position().top_right();
    }

    auto on_left_edge() -> Point
    {
        return aux_rect_position().top_left - as_displacement(child.size()).dx;
    }

    auto on_bottom_edge() -> Point
    {
        return aux_rect_position().bottom_left();
    }
};
}

TEST_F(PopupWindowPlacement, fixture_sets_up_parent_and_child)
{
    ASSERT_THAT(parent, Ne(null_window));
    ASSERT_THAT(parent.size(), Eq(initial_parent_size));
    ASSERT_THAT(basic_window_manager.info_for(parent).children(), ElementsAre(child));
    ASSERT_THAT(basic_window_manager.info_for(parent).type(), Eq(mir_window_type_normal));

    ASSERT_THAT(child, Ne(null_window));
    ASSERT_THAT(child.size(), Eq(initial_child_size));
    ASSERT_THAT(basic_window_manager.info_for(child).parent(), Eq(parent));
    ASSERT_THAT(basic_window_manager.info_for(child).type(), Eq(mir_window_type_menu));
}


/* From the Mir client API:
 * Positioning of the surface is specified with respect to the parent surface
 * via an adjacency rectangle. The server will attempt to choose an edge of the
 * adjacency rectangle on which to place the surface taking in to account
 * screen-edge proximity or similar constraints. In addition, the server can use
 * the edge affinity hint to consider only horizontal or only vertical adjacency
 * edges in the given rectangle.
 */

TEST_F(PopupWindowPlacement, given_aux_rect_away_from_right_side_edge_attachment_vertical_attaches_to_right_edge)
{
    modification = edge_attachment(rectangle_away_from_rhs, mir_edge_attachment_vertical);

    auto const expected_position = on_right_edge();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_right_side_edge_attachment_vertical_attaches_to_left_edge)
{
    modification = edge_attachment(rectangle_near_rhs, mir_edge_attachment_vertical);

    auto const expected_position = on_left_edge();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_both_sides_edge_attachment_vertical_attaches_to_right_edge)
{
    modification = edge_attachment(rectangle_near_both_sides, mir_edge_attachment_vertical);

    auto const expected_position = on_right_edge();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_away_from_bottom_edge_attachment_horizontal_attaches_to_bottom_edge)
{
    modification = edge_attachment(rectangle_away_from_bottom, mir_edge_attachment_horizontal);

    auto const expected_position = on_bottom_edge();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_bottom_edge_attachment_horizontal_attaches_to_top_edge)
{
    modification = edge_attachment(rectangle_near_bottom, mir_edge_attachment_horizontal);

    auto const expected_position = on_top_edge();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_both_sides_edge_attachment_any_attaches_to_bottom_edge)
{
    modification = edge_attachment(rectangle_near_both_sides, mir_edge_attachment_any);

    auto const expected_position = on_bottom_edge();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_both_sides_and_bottom_edge_attachment_any_attaches_to_top_edge)
{
    modification = edge_attachment(rectangle_near_both_sides_and_bottom, mir_edge_attachment_any);

    auto const expected_position = on_top_edge();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_all_sides_attachment_any_attaches_to_right_edge)
{
    modification = edge_attachment(rectangle_near_all_sides, mir_edge_attachment_any);

    auto const expected_position = on_right_edge();

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

namespace
{
MirPlacementGravity const all_gravities[] =
{
    mir_placement_gravity_northwest,
    mir_placement_gravity_north,
    mir_placement_gravity_northeast,
    mir_placement_gravity_west,
    mir_placement_gravity_center,
    mir_placement_gravity_east,
    mir_placement_gravity_southwest,
    mir_placement_gravity_south,
    mir_placement_gravity_southeast,
};

auto position_of(MirPlacementGravity rect_gravity, Rectangle rectangle) -> Point
{
    auto const displacement = as_displacement(rectangle.size);

    switch (rect_gravity)
    {
    case mir_placement_gravity_northwest:
        return rectangle.top_left;

    case mir_placement_gravity_north:
        return rectangle.top_left + Displacement{0.5 * displacement.dx, 0};

    case mir_placement_gravity_northeast:
        return rectangle.top_right();

    case mir_placement_gravity_west:
        return rectangle.top_left + Displacement{0, 0.5 * displacement.dy};

    case mir_placement_gravity_center:
        return rectangle.top_left + 0.5 * displacement;

    case mir_placement_gravity_east:
        return rectangle.top_right() + Displacement{0, 0.5 * displacement.dy};

    case mir_placement_gravity_southwest:
        return rectangle.bottom_left();

    case mir_placement_gravity_south:
        return rectangle.bottom_left() + Displacement{0.5 * displacement.dx, 0};

    case mir_placement_gravity_southeast:
        return rectangle.bottom_right();

    default:
        throw std::runtime_error("bad placement gravity");
    }

}
}

TEST_F(PopupWindowPlacement, given_no_hints_can_attach_by_every_gravity)
{
    modification.aux_rect() = Rectangle{{100, 50}, { 20, 20}};
    modification.placement_hints() = MirPlacementHints{};

    for (auto const rect_gravity : all_gravities)
    {
        modification.aux_rect_placement_gravity() = rect_gravity;

        auto const rect_anchor = position_of(rect_gravity, aux_rect_position());

        for (auto const window_gravity : all_gravities)
        {
            modification.window_placement_gravity() = window_gravity;

            EXPECT_CALL(*window_manager_policy, advise_move_to(_, _));
            basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);

            Rectangle child_rect{child.top_left(), child.size()};

            EXPECT_THAT(position_of(window_gravity, child_rect), Eq(rect_anchor))
                        << "rect_gravity=" << rect_gravity << ", window_gravity=" << window_gravity;
            Mock::VerifyAndClearExpectations(window_manager_policy);
        }
    }
}

TEST_F(PopupWindowPlacement, given_no_hints_can_attach_by_offset_at_every_gravity)
{
    auto const offset = Displacement{42, 13};

    modification.aux_rect() = Rectangle{{100, 50}, { 20, 20}};
    modification.placement_hints() = MirPlacementHints{};
    modification.aux_rect_placement_offset() = offset;

    for (auto const rect_gravity : all_gravities)
    {
        modification.aux_rect_placement_gravity() = rect_gravity;

        auto const rect_anchor = position_of(rect_gravity, aux_rect_position()) + offset;

        for (auto const window_gravity : all_gravities)
        {
            modification.window_placement_gravity() = window_gravity;

            EXPECT_CALL(*window_manager_policy, advise_move_to(_, _));
            basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);

            Rectangle child_rect{child.top_left(), child.size()};

            EXPECT_THAT(position_of(window_gravity, child_rect), Eq(rect_anchor))
                        << "rect_gravity=" << rect_gravity << ", window_gravity=" << window_gravity;
            Mock::VerifyAndClearExpectations(window_manager_policy);
        }
    }
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_right_side_and_offset_placement_is_flipped)
{
    DeltaX const x_offset{42};
    DeltaY const y_offset{13};

    modification.aux_rect() = rectangle_near_rhs;
    modification.placement_hints() = mir_placement_hints_flip_x;
    modification.aux_rect_placement_offset() = Displacement{x_offset, y_offset};
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_northeast;

    auto const expected_position = on_left_edge() + Displacement{-1*x_offset, y_offset};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_bottom_and_offset_placement_is_flipped)
{
    DeltaX const x_offset{42};
    DeltaY const y_offset{13};

    modification.aux_rect() = rectangle_near_bottom;
    modification.placement_hints() = mir_placement_hints_flip_y;
    modification.aux_rect_placement_offset() = Displacement{x_offset, y_offset};
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_southwest;

    auto const expected_position = on_top_edge() + Displacement{x_offset, -1*y_offset};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_bottom_right_and_offset_placement_is_flipped_both_ways)
{
    Displacement const displacement{42, 13};

    modification.aux_rect() = rectangle_near_both_bottom_right;
    modification.placement_hints() = mir_placement_hints_flip_any;
    modification.aux_rect_placement_offset() = displacement;
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_southeast;

    auto const expected_position = aux_rect_position().top_left - as_displacement(child.size()) - displacement;

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_right_side_placement_can_slide_in_x)
{
    modification.aux_rect() = rectangle_near_rhs;
    modification.placement_hints() = mir_placement_hints_slide_x;
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_northeast;

    Point const expected_position{display_area.top_right().x - as_displacement(child.size()).dx, aux_rect_position().top_left.y};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_left_side_placement_can_slide_in_x)
{
    Rectangle const rectangle_near_left_side{{0, 20}, {20, 20}};

    modification.aux_rect() = rectangle_near_left_side;
    modification.placement_hints() = mir_placement_hints_slide_x;
    modification.window_placement_gravity() = mir_placement_gravity_northeast;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_northwest;

    Point const expected_position{display_area.top_left.x, aux_rect_position().top_left.y};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_bottom_placement_can_slide_in_y)
{
    modification.aux_rect() = rectangle_near_bottom;
    modification.placement_hints() = mir_placement_hints_slide_y;
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_southwest;

    Point const expected_position{
        aux_rect_position().top_left.x,
        (display_area.bottom_left() - as_displacement(child.size())).y};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_top_placement_can_slide_in_y)
{
    modification.aux_rect() = rectangle_near_all_sides;
    modification.placement_hints() = mir_placement_hints_slide_y;
    modification.window_placement_gravity() = mir_placement_gravity_southwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_northwest;

    Point const expected_position{aux_rect_position().top_left.x, display_area.top_left.y};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_bottom_right_and_offset_placement_can_slide_in_x_and_y)
{
    modification.aux_rect() = rectangle_near_both_bottom_right;
    modification.placement_hints() = mir_placement_hints_slide_any;
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_southwest;

    auto const expected_position = display_area.bottom_right() - as_displacement(child.size());

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_right_side_placement_can_resize_in_x)
{
    modification.aux_rect() = rectangle_near_rhs;
    modification.placement_hints() = mir_placement_hints_resize_x;
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_northeast;

    auto const expected_position = aux_rect_position().top_right();
    Size const expected_size{as_size(display_area.bottom_right()-aux_rect_position().bottom_right()).width, child.size().height};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, expected_size));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(expected_size));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_left_side_placement_can_resize_in_x)
{
    Rectangle const rectangle_near_left_side{{0, 20}, {20, 20}};

    modification.aux_rect() = rectangle_near_left_side;
    modification.placement_hints() = mir_placement_hints_resize_x;
    modification.window_placement_gravity() = mir_placement_gravity_northeast;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_northwest;

    Point const expected_position{display_area.top_left.x, aux_rect_position().top_left.y};
    Size const expected_size{as_size(aux_rect_position().top_left-display_area.top_left).width, child.size().height};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, expected_size));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(expected_size));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_bottom_placement_can_resize_in_y)
{
    modification.aux_rect() = rectangle_near_bottom;
    modification.placement_hints() = mir_placement_hints_resize_y;
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_southwest;

    auto const expected_position = aux_rect_position().bottom_left();
    Size const expected_size{child.size().width, as_size(display_area.bottom_left()-aux_rect_position().bottom_left()).height};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, expected_size));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(expected_size));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_top_placement_can_resize_in_y)
{
    modification.aux_rect() = rectangle_near_all_sides;
    modification.placement_hints() = mir_placement_hints_resize_y;
    modification.window_placement_gravity() = mir_placement_gravity_southwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_northwest;

    Point const expected_position{aux_rect_position().top_left.x, display_area.top_left.y};
    Size const expected_size{child.size().width, as_size(aux_rect_position().top_left-display_area.top_left).height};

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, expected_size));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(expected_size));
}

TEST_F(PopupWindowPlacement, given_aux_rect_near_bottom_right_and_offset_placement_can_resize_in_x_and_y)
{
    modification.aux_rect() = rectangle_near_both_bottom_right;
    modification.placement_hints() = mir_placement_hints_resize_any;
    modification.window_placement_gravity() = mir_placement_gravity_northwest;
    modification.aux_rect_placement_gravity() = mir_placement_gravity_southeast;

    auto const expected_position = aux_rect_position().bottom_right();
    auto const expected_size = as_size(display_area.bottom_right()-expected_position);

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, expected_position));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, expected_size));
    basic_window_manager.modify_window(basic_window_manager.info_for(child), modification);
    ASSERT_THAT(child.top_left(), Eq(expected_position));
    ASSERT_THAT(child.size(), Eq(expected_size));
}
