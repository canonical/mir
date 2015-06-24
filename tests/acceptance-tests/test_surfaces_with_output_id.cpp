/*
 * Copyright © 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir/geometry/rectangle.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/shell/shell_wrapper.h"

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/any_surface.h"

#include "mir/test/validity_matchers.h"

#include <gtest/gtest.h>

#include <vector>
#include <algorithm>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

using namespace testing;

namespace
{
struct RectangleCompare
{
    bool operator()(geom::Rectangle const& rect1,
                    geom::Rectangle const& rect2)
    {
        int x1 = rect1.top_left.x.as_int();
        int y1 = rect1.top_left.y.as_int();
        int w1 = rect1.size.width.as_int();
        int h1 = rect1.size.height.as_int();

        int x2 = rect2.top_left.x.as_int();
        int y2 = rect2.top_left.y.as_int();
        int w2 = rect2.size.width.as_int();
        int h2 = rect2.size.height.as_int();

        return std::tie(x1,y1,w1,h1) < std::tie(x2,y2,w2,h2);
    }
};

class TrackingShell : public msh::ShellWrapper
{
public:
    using msh::ShellWrapper::ShellWrapper;

    mf::SurfaceId create_surface(
        std::shared_ptr<ms::Session> const& session,
        ms::SurfaceCreationParameters const& params) override
    {
        auto const surface = msh::ShellWrapper::create_surface(session, params);
        surfaces.push_back(session->surface(surface));
        return surface;
    }

    std::vector<geom::Rectangle> server_surface_rectangles()
    {
        std::vector<geom::Rectangle> rects;
        for (auto const& surface : surfaces)
        {
            if (auto const ss = surface.lock())
            {
                for (auto& renderable: ss->generate_renderables(this))
                    rects.push_back(renderable->screen_position());
            }
        }
        return rects;
    }

private:
    std::vector<std::weak_ptr<ms::Surface>> surfaces;
};

struct SurfacesWithOutputId : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        server.wrap_shell([this]
            (std::shared_ptr<msh::Shell> const& wrapped)
            ->std::shared_ptr<msh::Shell>
            {
                tracking_shell = std::make_shared<TrackingShell>(wrapped);
                return tracking_shell;
            });

        initial_display_layout(display_rects);
        mtf::ConnectedClientHeadlessServer::SetUp();
        ASSERT_THAT(tracking_shell, NotNull());

        config = mir_connection_create_display_config(connection);
        ASSERT_TRUE(config != NULL);
    }

    void TearDown() override
    {
        mir_display_config_destroy(config);
        tracking_shell.reset();
        mtf::ConnectedClientHeadlessServer::TearDown();
    }

    std::shared_ptr<MirSurface> create_non_fullscreen_surface_for(MirDisplayOutput const& output)
    {
        auto const& mode = output.modes[output.current_mode];

        auto spec = mir_connection_create_spec_for_normal_surface(connection, 
            static_cast<int>(mode.horizontal_resolution) - 1,
            static_cast<int>(mode.vertical_resolution) + 1,
            mir_pixel_format_abgr_8888);
        mir_surface_spec_set_fullscreen_on_output(spec, output.output_id);
        
        auto surface_raw = mir_surface_create_sync(spec);
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface_raw));
        mir_surface_spec_release(spec);

        return shared_ptr_surface(surface_raw);
    }

    std::shared_ptr<MirSurface> create_fullscreen_surface_for(MirDisplayOutput const& output)
    {
        auto const& mode = output.modes[output.current_mode];

        auto spec = mir_connection_create_spec_for_normal_surface(connection,
            static_cast<int>(mode.horizontal_resolution),
            static_cast<int>(mode.vertical_resolution),
            mir_pixel_format_abgr_8888);
        mir_surface_spec_set_fullscreen_on_output(spec, output.output_id);

        auto surface_raw = mir_surface_create_sync(spec);
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface_raw));
        mir_surface_spec_release(spec);

        return shared_ptr_surface(surface_raw);
    }

    std::shared_ptr<MirSurface> shared_ptr_surface(MirSurface* surface_raw)
    {
        return std::shared_ptr<MirSurface>{
            surface_raw,
            [](MirSurface* surface)
            {
                mir_surface_release_sync(surface);
            }};
    }

    std::vector<geom::Rectangle> const display_rects{
        {{0,0}, {800,600}},
        {{800,600}, {200,400}}};

    MirDisplayConfiguration* config;

    std::shared_ptr<TrackingShell> tracking_shell;
};

}

TEST_F(SurfacesWithOutputId, fullscreen_surfaces_are_placed_at_top_left_of_correct_output)
{
    std::vector<std::shared_ptr<MirSurface>> surfaces;

    for (uint32_t n = 0; n < config->num_outputs; ++n)
    {
        auto surface = create_fullscreen_surface_for(config->outputs[n]);
        EXPECT_TRUE(mir_surface_is_valid(surface.get()));
        surfaces.push_back(surface);
    }

    auto surface_rects = tracking_shell->server_surface_rectangles();
    auto sorted_display_rects = display_rects;
    std::sort(sorted_display_rects.begin(), sorted_display_rects.end(), RectangleCompare());
    std::sort(surface_rects.begin(), surface_rects.end(), RectangleCompare());
    EXPECT_EQ(sorted_display_rects, surface_rects);
}

TEST_F(SurfacesWithOutputId, requested_size_is_ignored_in_favour_of_display_size)
{
    using namespace testing;

    std::vector<std::pair<int, int>> expected_dimensions;
    std::vector<std::shared_ptr<MirSurface>> surfaces;
    for (uint32_t n = 0; n < config->num_outputs; ++n)
    {
        auto surface = create_non_fullscreen_surface_for(config->outputs[n]);

        EXPECT_THAT(surface.get(), IsValid());
        surfaces.push_back(surface);

        auto expected_mode = config->outputs[n].modes[config->outputs[n].current_mode];
        expected_dimensions.push_back(std::pair<int,int>{expected_mode.horizontal_resolution,
                                                         expected_mode.vertical_resolution});

    }

    auto surface_rects = tracking_shell->server_surface_rectangles();
    auto display_rects = this->display_rects;

    std::sort(display_rects.begin(), display_rects.end(), RectangleCompare());
    std::sort(surface_rects.begin(), surface_rects.end(), RectangleCompare());
    EXPECT_EQ(display_rects, surface_rects);
}
