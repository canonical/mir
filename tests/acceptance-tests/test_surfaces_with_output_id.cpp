/*
 * Copyright © 2013 Canonical Ltd.
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
#include "mir/compositor/scene.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/renderable.h"

#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_platform.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"

#include <gtest/gtest.h>

#include <vector>
#include <tuple>

namespace mtf = mir_test_framework;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay(std::vector<geom::Rectangle> const& rects)
        : rects{rects}
    {
        for (auto const& rect : rects)
        {
            display_buffers.push_back(
                std::make_shared<mtd::StubDisplayBuffer>(rect));
        }
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        for (auto& db : display_buffers)
            f(*db);
    }

    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        return std::unique_ptr<mg::DisplayConfiguration>(
            new mtd::StubDisplayConfig(rects)
        );
    }

private:
    std::vector<geom::Rectangle> const rects;
    std::vector<std::shared_ptr<mtd::StubDisplayBuffer>> display_buffers;
};

class StubPlatform : public mtd::NullPlatform
{
public:
    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/) override
    {
        return std::make_shared<mtd::StubBufferAllocator>();
    }

    std::shared_ptr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
        std::shared_ptr<mg::GLProgramFactory> const&,
        std::shared_ptr<mg::GLConfig> const&) override
    {
        return std::make_shared<StubDisplay>(display_rects());
    }

    static std::vector<geom::Rectangle> display_rects()
    {
        return {{{0,0}, {800,600}},
                {{800,600}, {200,400}}};
    }
};

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

struct ServerConfig : mtf::StubbedServerConfiguration
{
    std::shared_ptr<mg::Platform> the_graphics_platform() override
    {
        if (!graphics_platform)
            graphics_platform = std::make_shared<StubPlatform>();
        return graphics_platform;
    }

    std::shared_ptr<mg::Platform> graphics_platform;
};

using BasicFixture = mtf::BasicClientServerFixture<ServerConfig>;

struct SurfacesWithOutputId : BasicFixture
{
    void SetUp()
    {
        BasicFixture::SetUp();
        config = mir_connection_create_display_config(connection);
        ASSERT_TRUE(config != NULL);
    }

    void TearDown()
    {
        mir_display_config_destroy(config);
        BasicFixture::TearDown();
    }

    std::shared_ptr<MirSurface> create_non_fullscreen_surface_for(MirDisplayOutput const& output)
    {
        auto const& mode = output.modes[output.current_mode];

        MirSurfaceParameters const request_params =
        {
            __PRETTY_FUNCTION__,
            static_cast<int>(mode.horizontal_resolution) - 1,
            static_cast<int>(mode.vertical_resolution) + 1,
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware,
            output.output_id
        };

        auto surface_raw = mir_connection_create_surface_sync(connection, &request_params);
        return shared_ptr_surface(surface_raw);
    }

    std::shared_ptr<MirSurface> create_fullscreen_surface_for(MirDisplayOutput const& output)
    {
        auto const& mode = output.modes[output.current_mode];

        MirSurfaceParameters const request_params =
        {
            __PRETTY_FUNCTION__,
            static_cast<int>(mode.horizontal_resolution),
            static_cast<int>(mode.vertical_resolution),
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware,
            output.output_id
        };

        auto surface_raw = mir_connection_create_surface_sync(connection, &request_params);
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

    std::vector<geom::Rectangle> server_surface_rectangles()
    {
        std::vector<geom::Rectangle> rects;
        auto const renderables = server_config().the_scene()->renderable_list_for(this);
        for (auto const& renderable : renderables)
            rects.push_back(renderable->screen_position());
        return rects;
    }

    MirDisplayConfiguration* config;
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

    auto surface_rects = server_surface_rectangles();
    auto display_rects = StubPlatform::display_rects();

    std::sort(display_rects.begin(), display_rects.end(), RectangleCompare());
    std::sort(surface_rects.begin(), surface_rects.end(), RectangleCompare());
    EXPECT_EQ(display_rects, surface_rects);
}

TEST_F(SurfacesWithOutputId, non_fullscreen_surfaces_are_not_accepted)
{
    for (uint32_t n = 0; n < config->num_outputs; ++n)
    {
        auto surface = create_non_fullscreen_surface_for(config->outputs[n]);

        EXPECT_FALSE(mir_surface_is_valid(surface.get()));
    }
}
