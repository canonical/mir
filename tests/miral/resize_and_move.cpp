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
 */

#include "test_window_manager_tools.h"

using namespace miral;
using namespace testing;
namespace mt = mir::test;
namespace mg = mir::graphics;

namespace
{
Rectangle const display_area{{}, {1280, 720}};

struct ResizeAndMove : mt::TestWindowManagerTools
{
    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    auto create_window(mir::shell::SurfaceSpecification creation_parameters) -> Window
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
};

}

TEST_F(ResizeAndMove, can_move_window)
{
    Point const start{20, 20};
    Point const end{200, 340};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.top_left = start;
        params.set_size({50, 50});
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.top_left() = end;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.top_left(), Eq(end));
}

TEST_F(ResizeAndMove, can_resize)
{
    Size const start{100, 100};
    Size const end{48, 320};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.size() = end;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(end));
}

TEST_F(ResizeAndMove, can_not_be_resized_smaller_than_min_size)
{
    Size const start{100, 200};
    Size const min{75, 80};
    Size const attempt{50, 60};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        params.min_width = min.width;
        params.min_height = min.height;
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.size() = attempt;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(min));
}

TEST_F(ResizeAndMove, can_not_be_resized_larger_than_max_size)
{
    Size const start{100, 200};
    Size const max{320, 250};
    Size const attempt{600, 700};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        params.max_width = max.width;
        params.max_height = max.height;
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.size() = attempt;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(max));
}

TEST_F(ResizeAndMove, min_width_can_be_set_without_height)
{
    Size const start{100, 200};
    Width const min_width{75};
    Size const attempt{50, 60};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        params.min_width = min_width;
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.size() = attempt;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(Size{min_width, attempt.height}));
}

TEST_F(ResizeAndMove, min_height_can_be_set_without_width)
{
    Size const start{100, 200};
    Height const min_height{80};
    Size const attempt{50, 60};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        params.min_height = min_height;
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.size() = attempt;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(Size{attempt.width, min_height}));
}

TEST_F(ResizeAndMove, max_width_can_be_set_without_height)
{
    Size const start{100, 200};
    Width const max_width{300};
    Size const attempt{400, 450};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        params.max_width = max_width;
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.size() = attempt;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(Size{max_width, attempt.height}));
}

TEST_F(ResizeAndMove, max_height_can_be_set_without_width)
{
    Size const start{100, 200};
    Height const max_height{250};
    Size const attempt{400, 450};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        params.max_height = max_height;
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.size() = attempt;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(Size{attempt.width, max_height}));
}

TEST_F(ResizeAndMove, min_size_applied_when_set)
{
    Size const start{50, 60};
    Size const min{75, 80};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.min_width() = min.width;
        spec.min_height() = min.height;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(min));
}

TEST_F(ResizeAndMove, max_size_applied_when_set)
{
    Size const start{600, 700};
    Size const max{320, 250};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.max_width() = max.width;
        spec.max_height() = max.height;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(max));
}

TEST_F(ResizeAndMove, min_size_respected_when_set_in_same_commit_as_resize)
{
    Size const start{100, 200};
    Size const min{75, 80};
    Size const attempt{50, 60};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.min_width() = min.width;
        spec.min_height() = min.height;
        spec.size() = attempt;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(min));
}

TEST_F(ResizeAndMove, max_size_respected_when_set_in_same_commit_as_resize)
{
    Size const start{50, 50};
    Size const max{320, 250};
    Size const attempt{600, 700};

    Window window;
    {
        mir::shell::SurfaceSpecification params;
        params.set_size(start);
        window = create_window(params);
    }

    {
        WindowSpecification spec;
        spec.max_width() = max.width;
        spec.max_height() = max.height;
        spec.size() = attempt;
        window_manager_tools.modify_window(window, spec);
    }

    EXPECT_THAT(window.size(), Eq(max));
}

// TODO: test that window is positioned correctly on clamped resize in all directions
