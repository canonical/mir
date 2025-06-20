/*
 * Copyright © Canonical Ltd.
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

#include "render_scene_into_surface.h"

#include <miral/test_server.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir/server.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "mir/scene/surface.h"

namespace mtf = mir_test_framework;

using namespace testing;
using namespace miral;

// TODO (mattkae): The platforms are captured here so that the libraries
//  do not get released before we hit deconstruction, specifically for the
//  output surface in the basic screen shooter.
std::vector<std::shared_ptr<mir::graphics::RenderingPlatform>> platforms;
std::vector<std::shared_ptr<mir::graphics::DisplayPlatform>> displays;

class RenderSceneIntoSurfaceTest : public TestServer
{
public:
    RenderSceneIntoSurfaceTest()
    {
        start_server_in_setup = false;
        add_start_callback([&]()
        {
            platforms = server().the_rendering_platforms();
            displays = server().the_display_platforms();
        });
    }
};

TEST_F(RenderSceneIntoSurfaceTest, set_capture_area_before_starting)
{
    RenderSceneIntoSurface render_scene_into_surface;
    add_server_init(render_scene_into_surface);
    render_scene_into_surface.capture_area(Rectangle(
        Point(100, 100),
        Size(200, 200)
    ));
    add_start_callback([&]
    {
        auto const scene = server().the_scene();
        auto const elements = scene->scene_elements_for(this);
        EXPECT_THAT(elements.size(), Eq(1));
        auto const element = elements.at(0);
        EXPECT_THAT(element->renderable()->screen_position(), Eq(Rectangle(
            Point(0, 0),
            Size(200, 200)
        )));
    });
    start_server();
}

TEST_F(RenderSceneIntoSurfaceTest, set_capture_area_after_starting)
{
    RenderSceneIntoSurface render_scene_into_surface;
    add_server_init(render_scene_into_surface);

    add_start_callback([&]
    {
        render_scene_into_surface.capture_area(Rectangle(
            Point(100, 100),
            Size(200, 200)
        ));
        auto const scene = server().the_scene();
        auto const elements = scene->scene_elements_for(this);
        EXPECT_THAT(elements.size(), Eq(1));
        auto const element = elements.at(0);
        EXPECT_THAT(element->renderable()->screen_position(), Eq(Rectangle(
            Point(0, 0),
            Size(200, 200)
        )));
    });
    start_server();
}

TEST_F(RenderSceneIntoSurfaceTest, set_overlay_cursor_before_starting)
{
    RenderSceneIntoSurface render_scene_into_surface;
    render_scene_into_surface.overlay_cursor(true);
    add_server_init(render_scene_into_surface);
    start_server();
}

TEST_F(RenderSceneIntoSurfaceTest, set_overlay_cursor_after_starting)
{
    RenderSceneIntoSurface render_scene_into_surface;
    add_server_init(render_scene_into_surface);

    add_start_callback([&]
    {
        render_scene_into_surface.overlay_cursor(true);
    });
    start_server();
}

TEST_F(RenderSceneIntoSurfaceTest, on_surface_ready_callback_called)
{
    bool called = false;
    RenderSceneIntoSurface render_scene_into_surface;
    add_server_init(render_scene_into_surface);
    render_scene_into_surface.on_surface_ready([&](auto const&)
    {
        called = true;
    });
    start_server();
    EXPECT_THAT(called, Eq(true));
}

TEST_F(RenderSceneIntoSurfaceTest, surface_never_contains_point)
{
    RenderSceneIntoSurface render_scene_into_surface;
    add_server_init(render_scene_into_surface);
    render_scene_into_surface.on_surface_ready([&](std::shared_ptr<mir::scene::Surface> const&surface)
    {
       EXPECT_THAT(surface->input_area_contains(Point(0, 0)), Eq(false));
    });
    start_server();
}