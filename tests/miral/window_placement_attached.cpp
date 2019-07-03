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
