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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "test_window_manager_tools.h"

using namespace miral;
using namespace testing;
namespace mt = mir::test;
namespace mg = mir::graphics;

namespace
{
X const display_left{30};
Y const display_top{40};
Width const display_width{1280};
Height const display_height{720};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

// there is some wierdness overloading the raw MirPlacementGravity stream operator in Clang, so we wrap it in a struct
struct AttachedEdges
{
    AttachedEdges(MirPlacementGravity edges) : edges{edges} {}
    AttachedEdges(int edges) : AttachedEdges((MirPlacementGravity)edges) {}
    operator MirPlacementGravity() const { return edges; }
    auto operator==(AttachedEdges const& other) const -> bool { return edges == other.edges; }
    MirPlacementGravity edges;
};

std::ostream &operator <<(std::ostream &o, AttachedEdges edges)
{
    o << "MirPlacementGravity{center";
    if (edges & mir_placement_gravity_north)
        o << " | north";
    if (edges & mir_placement_gravity_south)
        o << " | south";
    if (edges & mir_placement_gravity_east)
        o << " | east";
    if (edges & mir_placement_gravity_west)
        o << " | west";
    o << "}";
    return o;
}

struct WindowPlacementAttached : mt::TestWindowManagerTools, WithParamInterface<AttachedEdges>
{
    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
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

        // Clear the expectations used to capture the window
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return result;
    }

    auto placement_for_attachement(Rectangle output_rect, Size window_size, MirPlacementGravity edges) -> Rectangle
    {
        std::experimental::optional<X> left;
        std::experimental::optional<Y> top;
        std::experimental::optional<X> right;
        std::experimental::optional<Y> bottom;

        if (edges & mir_placement_gravity_west)
            left = output_rect.left();
        if (edges & mir_placement_gravity_east)
            right = output_rect.right();
        if (edges & mir_placement_gravity_north)
            top = output_rect.top();
        if (edges & mir_placement_gravity_south)
            bottom = output_rect.bottom();

        if (!left)
        {
            if (right)
                left = right.value() - DeltaX{window_size.width.as_int()};
            else
                left = output_rect.left() + DeltaX{(output_rect.size.width - window_size.width).as_int() / 2};
        }
        if (!right)
            right = left.value() + DeltaX{window_size.width.as_int()};

        if (!top)
        {
            if (bottom)
                top = bottom.value() - DeltaY{window_size.height.as_int()};
            else
                top = output_rect.top() + DeltaY{(output_rect.size.height - window_size.height).as_int() / 2};
        }
        if (!bottom)
            bottom = top.value() + DeltaY{window_size.height.as_int()};

        Point top_left{left.value(), top.value()};
        Size size{as_size(Displacement{right.value() - left.value(), bottom.value() - top.value()})};

        return Rectangle{top_left, size};
    }

    auto apply_exclusive_zone(
        Rectangle zone,
        Rectangle exclusive_rect,
        Size window_size,
        MirPlacementGravity edges) -> Rectangle
    {
        if ((edges & mir_placement_gravity_east) && (edges & mir_placement_gravity_west))
            edges = (MirPlacementGravity)(edges & ~(mir_placement_gravity_east | mir_placement_gravity_west));

        if ((edges & mir_placement_gravity_north) && (edges & mir_placement_gravity_south))
            edges = (MirPlacementGravity)(edges & ~(mir_placement_gravity_north | mir_placement_gravity_south));

        switch (edges)
        {
        case mir_placement_gravity_west:
            zone.top_left.x += as_delta(exclusive_rect.right());
            zone.size.width -= as_delta(exclusive_rect.right());
            break;

        case mir_placement_gravity_east:
            zone.size.width -= as_delta(window_size.width) - as_delta(exclusive_rect.left());
            break;

        case mir_placement_gravity_north:
            zone.top_left.y += as_delta(exclusive_rect.bottom());
            zone.size.height -= as_delta(exclusive_rect.bottom());
            break;

        case mir_placement_gravity_south:
            zone.size.height -= as_delta(window_size.height) - as_delta(exclusive_rect.top());
            break;

        default:
            break;
        }

        return zone;
    }
};

}

TEST_P(WindowPlacementAttached, window_is_initially_placed_correctly)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    Rectangle placement{placement_for_attachement(display_area, window_size, edges)};
    AttachedEdges actual_edges = info.attached_edges();

    EXPECT_THAT(info.state(), Eq(mir_window_state_attached));
    EXPECT_THAT(actual_edges, Eq(edges));
    EXPECT_THAT(window.top_left(), Eq(placement.top_left));
    EXPECT_THAT(window.size(), Eq(placement.size));
}

TEST_P(WindowPlacementAttached, window_is_placed_correctly_when_attached_edges_change)
{
    AttachedEdges edges = GetParam();
    Size window_size{70, 90};

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = mir_placement_gravity_southwest;
        params.size = window_size;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    WindowSpecification spec;
    spec.attached_edges() = edges;
    window_manager_tools.modify_window(window, spec);

    Rectangle placement{placement_for_attachement(display_area, window_size, edges)};
    AttachedEdges actual_edges = info.attached_edges();

    EXPECT_THAT(info.state(), Eq(mir_window_state_attached));
    EXPECT_THAT(actual_edges, Eq(edges));
    EXPECT_THAT(window.top_left(), Eq(placement.top_left));
    EXPECT_THAT(window.size(), Eq(placement.size));
}

TEST_P(WindowPlacementAttached, window_is_placed_correctly_when_size_changes)
{
    AttachedEdges edges = GetParam();
    Size initial_size{70, 90};
    Size new_size{140, 80};

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = initial_size;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    WindowSpecification spec;
    spec.size() = new_size;
    window_manager_tools.modify_window(window, spec);

    Rectangle placement{placement_for_attachement(display_area, new_size, edges)};
    AttachedEdges actual_edges = info.attached_edges();

    EXPECT_THAT(info.state(), Eq(mir_window_state_attached));
    EXPECT_THAT(actual_edges, Eq(edges));
    EXPECT_THAT(window.top_left(), Eq(placement.top_left));
    EXPECT_THAT(window.size(), Eq(placement.size));
}

TEST_P(WindowPlacementAttached, window_is_placed_correctly_when_output_changes)
{
    AttachedEdges edges = GetParam();
    Size window_size{70, 90};
    Rectangle new_display_area{{120, 18}, {600, 700}};

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    notify_configuration_applied(create_fake_display_configuration({new_display_area}));

    Rectangle placement{placement_for_attachement(new_display_area, window_size, edges)};
    AttachedEdges actual_edges = info.attached_edges();

    EXPECT_THAT(info.state(), Eq(mir_window_state_attached));
    EXPECT_THAT(actual_edges, Eq(edges));
    EXPECT_THAT(window.top_left(), Eq(placement.top_left));
    EXPECT_THAT(window.size(), Eq(placement.size));
}

TEST_P(WindowPlacementAttached, window_is_placed_correctly_when_put_in_attached_state)
{
    AttachedEdges edges = GetParam();
    Point initial_top_left{60, 80};
    Size window_size{100, 110};

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_restored;
        params.attached_edges = edges;
        params.size = window_size;
        params.top_left = initial_top_left;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);

    Rectangle placement{placement_for_attachement(display_area, window_size, edges)};

    EXPECT_THAT(info.state(), Eq(mir_window_state_restored));
    EXPECT_THAT(window.top_left(), Ne(placement.top_left));

    WindowSpecification spec;
    spec.state() = mir_window_state_attached;
    window_manager_tools.modify_window(window, spec);

    AttachedEdges actual_edges = info.attached_edges();

    EXPECT_THAT(info.state(), Eq(mir_window_state_attached));
    EXPECT_THAT(actual_edges, Eq(edges));
    EXPECT_THAT(window.top_left(), Eq(placement.top_left));
    EXPECT_THAT(window.size(), Eq(placement.size));
}

TEST_P(WindowPlacementAttached, maximized_window_respects_exclusive_zone)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};
    Rectangle exclusive_rect{{0, 0}, window_size};

    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        normal = create_window(params);
    }
    Rectangle zone = apply_exclusive_zone(display_area, exclusive_rect, window_size, edges);

    EXPECT_THAT(normal.top_left(), Eq(zone.top_left));
    EXPECT_THAT(normal.size(), Eq(zone.size));
}

TEST_P(WindowPlacementAttached, window_respects_exclusive_zone_when_maximized)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};
    Rectangle exclusive_rect{{0, 0}, window_size};

    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_restored;
        normal = create_window(params);
    }

    mir::shell::SurfaceSpecification spec;
    spec.state = mir_window_state_maximized;
    basic_window_manager.modify_surface(session, normal, spec);

    Rectangle zone = apply_exclusive_zone(display_area, exclusive_rect, window_size, edges);

    EXPECT_THAT(normal.top_left(), Eq(zone.top_left));
    EXPECT_THAT(normal.size(), Eq(zone.size));
}

TEST_P(WindowPlacementAttached, fullscreen_window_ignores_exclusive_zone)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};
    Rectangle exclusive_rect{{0, 0}, window_size};

    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_fullscreen;
        normal = create_window(params);
    }

    EXPECT_THAT(normal.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(normal.size(), Eq(display_area.size));
}

TEST_P(WindowPlacementAttached, window_ignores_exclusive_zone_when_fullscreened)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};
    Rectangle exclusive_rect{{0, 0}, window_size};

    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_restored;
        normal = create_window(params);
    }

    mir::shell::SurfaceSpecification spec;
    spec.state = mir_window_state_fullscreen;
    basic_window_manager.modify_surface(session, normal, spec);

    EXPECT_THAT(normal.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(normal.size(), Eq(display_area.size));
}

TEST_P(WindowPlacementAttached, stretched_attached_window_ignores_exclusive_zone)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};
    Rectangle exclusive_rect{{0, 0}, window_size};

    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        create_window(params);
    }

    Window attached;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = (MirPlacementGravity)(
            mir_placement_gravity_north |
            mir_placement_gravity_south |
            mir_placement_gravity_east |
            mir_placement_gravity_west);
        attached = create_window(params);
    }
    Rectangle without_exclusive_zone = display_area;

    EXPECT_THAT(attached.top_left(), Eq(without_exclusive_zone.top_left));
    EXPECT_THAT(attached.size(), Eq(without_exclusive_zone.size));
}

TEST_P(WindowPlacementAttached, maximized_window_respects_multiple_stacked_exclusive_zones)
{
    AttachedEdges edges = GetParam();
    Size window_a_size{120, 80};
    Size window_b_size{50, 20};
    Rectangle exclusive_rect_a{{0, 0}, window_a_size};
    Rectangle exclusive_rect_b{{0, 0}, window_b_size};

    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_a_size;
        params.exclusive_rect = exclusive_rect_a;
        create_window(params);
    }

    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_b_size;
        params.exclusive_rect = exclusive_rect_b;
        create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        normal = create_window(params);
    }
    Rectangle zone = apply_exclusive_zone(display_area, exclusive_rect_a, window_a_size, edges);
    zone = apply_exclusive_zone(zone, exclusive_rect_b, window_b_size, edges);

    EXPECT_THAT(normal.top_left(), Eq(zone.top_left));
    EXPECT_THAT(normal.size(), Eq(zone.size));
}

TEST_P(WindowPlacementAttached, maximized_window_respects_exclusive_zone_smaller_than_window)
{
    AttachedEdges edges = GetParam();
    Size window_size{200, 180};
    Rectangle exclusive_rect{{20, 30}, {130, 60}};

    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        normal = create_window(params);
    }
    Rectangle zone = apply_exclusive_zone(display_area, exclusive_rect, window_size, edges);

    EXPECT_THAT(normal.top_left(), Eq(zone.top_left));
    EXPECT_THAT(normal.size(), Eq(zone.size));
}

TEST_P(WindowPlacementAttached, exclusive_zone_is_cleared_when_window_is_removed)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};
    Rectangle exclusive_rect{{0, 0}, window_size};

    Window attached;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        attached = create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        normal = create_window(params);
    }

    basic_window_manager.remove_surface(session, attached);

    EXPECT_THAT(normal.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(normal.size(), Eq(display_area.size));
}

TEST_P(WindowPlacementAttached, exclusive_zone_is_cleared_when_window_becomes_non_attached)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};
    Rectangle exclusive_rect{{0, 0}, window_size};

    Window attached;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        attached = create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        normal = create_window(params);
    }

    mir::shell::SurfaceSpecification spec;
    spec.state = mir_window_state_restored;
    basic_window_manager.modify_surface(session, attached, spec);

    EXPECT_THAT(normal.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(normal.size(), Eq(display_area.size));
}

TEST_P(WindowPlacementAttached, exclusive_zone_is_cleared_when_exclusive_rect_cleared)
{
    AttachedEdges edges = GetParam();
    Size window_size{120, 80};
    Rectangle exclusive_rect{{0, 0}, window_size};

    Window attached;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_attached;
        params.attached_edges = edges;
        params.size = window_size;
        params.exclusive_rect = exclusive_rect;
        attached = create_window(params);
    }

    Window normal;
    {
        mir::scene::SurfaceCreationParameters params;
        params.state = mir_window_state_maximized;
        normal = create_window(params);
    }

    mir::shell::SurfaceSpecification spec;
    spec.exclusive_rect = mir::optional_value<mir::optional_value<Rectangle>>{{}};
    basic_window_manager.modify_surface(session, attached, spec);

    EXPECT_THAT(normal.top_left(), Eq(display_area.top_left));
    EXPECT_THAT(normal.size(), Eq(display_area.size));
}

INSTANTIATE_TEST_CASE_P(WindowPlacementAttached, WindowPlacementAttached, ::testing::Values(
    mir_placement_gravity_center,
    mir_placement_gravity_west,
    mir_placement_gravity_east,
    mir_placement_gravity_north,
    mir_placement_gravity_south,
    mir_placement_gravity_northwest,
    mir_placement_gravity_northeast,
    mir_placement_gravity_southwest,
    mir_placement_gravity_southeast,
    mir_placement_gravity_west | mir_placement_gravity_east,
    mir_placement_gravity_west | mir_placement_gravity_east | mir_placement_gravity_north,
    mir_placement_gravity_west | mir_placement_gravity_east | mir_placement_gravity_south,
    mir_placement_gravity_north | mir_placement_gravity_south,
    mir_placement_gravity_north | mir_placement_gravity_south | mir_placement_gravity_east,
    mir_placement_gravity_north | mir_placement_gravity_south | mir_placement_gravity_west,
    mir_placement_gravity_north | mir_placement_gravity_south | mir_placement_gravity_east | mir_placement_gravity_west
));
