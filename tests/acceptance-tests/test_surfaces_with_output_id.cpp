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
#include "mir/compositor/compositing_criteria.h"
#include "mir/geometry/rectangle.h"

#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_platform.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test/cross_process_action.h"

#include <gtest/gtest.h>

#include <thread>
#include <vector>
#include <tuple>

namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{
char const* const mir_test_socket = mtf::test_socket_file().c_str();

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

    std::shared_ptr<mg::DisplayConfiguration> configuration() override
    {
        return std::make_shared<mtd::StubDisplayConfig>(rects);
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
        std::shared_ptr<mg::DisplayConfigurationPolicy> const&) override
    {
        return std::make_shared<StubDisplay>(display_rects());
    }

    std::vector<geom::Rectangle> display_rects()
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

}

using SurfacesWithOutputId = BespokeDisplayServerTestFixture;

TEST_F(SurfacesWithOutputId, fullscreen_surfaces_are_placed_at_top_left_of_correct_output)
{
    mt::CrossProcessAction client_connect_and_create_surface;
    mt::CrossProcessAction client_release_surface_and_disconnect;
    mt::CrossProcessAction verify_scene;

    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig(mt::CrossProcessAction verify_scene)
            : verify_scene{verify_scene}
        {
        }

        std::shared_ptr<mg::Platform> the_graphics_platform() override
        {
            if (!graphics_platform)
                graphics_platform = std::make_shared<StubPlatform>();
            return graphics_platform;
        }

        void exec() override
        {
            verification_thread = std::thread([this](){
                verify_scene.exec([this]
                {
                    auto scene = the_scene();

                    struct VerificationFilter : public mc::FilterForScene
                    {
                        bool operator()(mc::CompositingCriteria const& criteria)
                        {
                            auto const& trans = criteria.transformation();
                            int width = trans[0][0];
                            int height = trans[1][1];
                            int x = trans[3][0] - width / 2;
                            int y = trans[3][1] - height / 2;
                            rects.push_back({{x, y}, {width, height}});
                            return false;
                        }

                        std::vector<geom::Rectangle> rects;
                    } filter;

                    struct NullOperator : public mc::OperatorForScene
                    {
                        void operator()(mc::CompositingCriteria const&,
                                        mc::BufferStream&) {}
                    } null_operator;

                    scene->for_each_if(filter, null_operator);

                    auto display_rects = graphics_platform->display_rects();
                    std::sort(display_rects.begin(), display_rects.end(), RectangleCompare());
                    std::sort(filter.rects.begin(), filter.rects.end(), RectangleCompare());
                    EXPECT_EQ(display_rects, filter.rects);
                });
            });
        }

        void on_exit() override
        {
            verification_thread.join();
        }

        std::shared_ptr<StubPlatform> graphics_platform;
        std::thread verification_thread;
        mt::CrossProcessAction verify_scene;
    } server_config{verify_scene};

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(mt::CrossProcessAction const& connect_and_create_surface,
                     mt::CrossProcessAction const& release_surface_and_disconnect)
            : connect_and_create_surface{connect_and_create_surface},
              release_surface_and_disconnect{release_surface_and_disconnect}
        {
        }

        void exec()
        {
            MirConnection* connection;
            std::vector<MirSurface*> surfaces;

            connect_and_create_surface.exec([&]
            {
                connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
                ASSERT_TRUE(mir_connection_is_valid(connection));

                auto config = mir_connection_create_display_config(connection);
                ASSERT_TRUE(config != NULL);

                for (uint32_t n = 0; n < config->num_outputs; ++n)
                {
                    auto const& output = config->outputs[n];
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

                    surfaces.push_back(
                        mir_connection_create_surface_sync(connection, &request_params));
                }

                mir_display_config_destroy(config);
            });

            release_surface_and_disconnect.exec([&]
            {
                for (auto surface : surfaces)
                    mir_surface_release_sync(surface);
                mir_connection_release(connection);
            });
        }

        mt::CrossProcessAction connect_and_create_surface;
        mt::CrossProcessAction release_surface_and_disconnect;
    } client_config{client_connect_and_create_surface,
                    client_release_surface_and_disconnect};

    launch_client_process(client_config);

    run_in_test_process([&]
    {
        client_connect_and_create_surface();
        verify_scene();
        client_release_surface_and_disconnect();
    });
}

TEST_F(SurfacesWithOutputId, non_fullscreen_surfaces_are_not_accepted)
{
    mt::CrossProcessAction client_connect_and_create_scene;
    mt::CrossProcessAction client_disconnect;

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform() override
        {
            if (!graphics_platform)
                graphics_platform = std::make_shared<StubPlatform>();
            return graphics_platform;
        }

        std::shared_ptr<StubPlatform> graphics_platform;
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            ASSERT_TRUE(mir_connection_is_valid(connection));

            auto config = mir_connection_create_display_config(connection);
            ASSERT_TRUE(config != NULL);

            for (uint32_t n = 0; n < config->num_outputs; ++n)
            {
                auto const& output = config->outputs[n];
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

                auto surface = mir_connection_create_surface_sync(connection, &request_params);
                EXPECT_FALSE(mir_surface_is_valid(surface));
            }

            mir_display_config_destroy(config);

            mir_connection_release(connection);
        }

    } client_config;

    launch_client_process(client_config);
}
