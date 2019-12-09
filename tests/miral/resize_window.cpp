/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

struct ResizeParam
{
    MirResizeEdge edge;
    bool grow;
};

std::ostream& operator<<(std::ostream& ostream, ResizeParam const& param)
{
    std::string edge = "???";
    switch (param.edge)
    {
    case mir_resize_edge_none: edge = "none"; break;
    case mir_resize_edge_north: edge = "north"; break;
    case mir_resize_edge_south: edge = "south"; break;
    case mir_resize_edge_east: edge = "east"; break;
    case mir_resize_edge_west: edge = "west"; break;
    case mir_resize_edge_northeast: edge = "northeast"; break;
    case mir_resize_edge_northwest: edge = "northwest"; break;
    case mir_resize_edge_southeast: edge = "southeast"; break;
    case mir_resize_edge_southwest: edge = "southwest"; break;
    }
    return ostream << (param.grow ? "grow" : "shrink") << " " << edge;
}

struct ResizeWindow : mt::TestWindowManagerTools, WithParamInterface<ResizeParam>
{
    Size initial_size{50, 40};
    Size large_size{107, 120}; // larger than max
    Size small_size{22, 19}; // smaller than min
    Size max_size{80, 75};
    Size min_size{32, 28};
    miral::Window window;

    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
        window = create_window();
    }

    auto create_window() -> Window
    {
        Window window;

        mir::scene::SurfaceCreationParameters creation_parameters;
        creation_parameters.type = mir_window_type_normal;
        creation_parameters.top_left = {200, 300};
        creation_parameters.size = initial_size;

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [&window](WindowInfo const& window_info)
                        { window = window_info.window(); }));

        basic_window_manager.add_surface(session, creation_parameters, &create_surface);
        basic_window_manager.select_active_window(window);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return window;
    }

    auto top_left_after_resize(miral::Window const& window, Size const& new_size, MirResizeEdge edge) -> Point
    {
        Size old_size = window.size();
        Point result = window.top_left();

        if (edge | mir_resize_edge_west)
            result.x += old_size.width - new_size.width;

        if (edge | mir_resize_edge_north)
            result.y += old_size.height - new_size.height;

        return result;
    }
};
}

TEST_P(ResizeWindow, can_resize)
{
    auto const edge = GetParam().edge;
    auto const new_size = GetParam().grow ? large_size : small_size;
    auto const new_top_left = top_left_after_resize(window, new_size, edge);

    {
        miral::WindowSpecification modifications;
        modifications.size() = new_size;
        modifications.top_left() = new_top_left;
        window_manager_tools.modify_window(window, modifications);
    }

    EXPECT_THAT(window.size(), Eq(new_size));
    EXPECT_THAT(window.top_left(), Eq(new_top_left));
}

TEST_F(ResizeWindow, clamped_when_max_size_applied)
{
    auto const excessive_size = large_size;
    auto const clamp_size = max_size;

    {
        miral::WindowSpecification modifications;
        modifications.size() = excessive_size;
        window_manager_tools.modify_window(window, modifications);
    }

    ASSERT_THAT(window.size(), Eq(excessive_size)) << "Precondition failed";

    {
        miral::WindowSpecification modifications;
        modifications.max_width() = clamp_size.width;
        modifications.max_height() = clamp_size.height;
        window_manager_tools.modify_window(window, modifications);
    }

    EXPECT_THAT(window.size(), Eq(clamp_size));
}

TEST_F(ResizeWindow, clamped_when_min_size_applied)
{
    auto const excessive_size = small_size;
    auto const clamp_size = min_size;

    {
        miral::WindowSpecification modifications;
        modifications.size() = excessive_size;
        window_manager_tools.modify_window(window, modifications);
    }

    ASSERT_THAT(window.size(), Eq(excessive_size)) << "Precondition failed";

    {
        miral::WindowSpecification modifications;
        modifications.min_width() = clamp_size.width;
        modifications.min_height() = clamp_size.height;
        window_manager_tools.modify_window(window, modifications);
    }

    EXPECT_THAT(window.size(), Eq(clamp_size));
}

TEST_P(ResizeWindow, clamped_on_resize)
{
    auto const edge = GetParam().edge;
    auto const excessive_size = GetParam().grow ? large_size : small_size;
    auto const clamp_size = GetParam().grow ? max_size : min_size;
    auto const requested_new_top_left = top_left_after_resize(window, excessive_size, edge);
    auto const correct_new_top_left = top_left_after_resize(window, clamp_size, edge);

    {
        miral::WindowSpecification modifications;
        if (GetParam().grow)
        {
            modifications.max_width() = clamp_size.width;
            modifications.max_height() = clamp_size.height;
        }
        else
        {
            modifications.min_width() = clamp_size.width;
            modifications.min_height() = clamp_size.height;
        }
        window_manager_tools.modify_window(window, modifications);
    }

    {
        miral::WindowSpecification modifications;
        modifications.size() = excessive_size;
        modifications.top_left() = requested_new_top_left;
        window_manager_tools.modify_window(window, modifications);
    }

    EXPECT_THAT(window.size(), Eq(clamp_size));
    EXPECT_THAT(window.top_left(), Eq(correct_new_top_left));
}

INSTANTIATE_TEST_CASE_P(ResizeLarger, ResizeWindow, ::testing::Values(
    ResizeParam{mir_resize_edge_north, true},
    ResizeParam{mir_resize_edge_south, true},
    ResizeParam{mir_resize_edge_east, true},
    ResizeParam{mir_resize_edge_west, true},
    ResizeParam{mir_resize_edge_northeast, true},
    ResizeParam{mir_resize_edge_northwest, true},
    ResizeParam{mir_resize_edge_southeast, true},
    ResizeParam{mir_resize_edge_southwest, true}));

INSTANTIATE_TEST_CASE_P(ResizeSmaller, ResizeWindow, ::testing::Values(
    ResizeParam{mir_resize_edge_north, false},
    ResizeParam{mir_resize_edge_south, false},
    ResizeParam{mir_resize_edge_east, false},
    ResizeParam{mir_resize_edge_west, false},
    ResizeParam{mir_resize_edge_northeast, false},
    ResizeParam{mir_resize_edge_northwest, false},
    ResizeParam{mir_resize_edge_southeast, false},
    ResizeParam{mir_resize_edge_southwest, false}));
