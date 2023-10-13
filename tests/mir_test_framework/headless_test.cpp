/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir_test_framework/headless_test.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/headless_display_buffer_compositor_factory.h"

#include "mir/shared_library.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display.h"
#include "mir_test_framework/executable_path.h"

#include <boost/throw_exception.hpp>

namespace geom = mir::geometry;
namespace mtf = mir_test_framework;
namespace mg = mir::graphics;

namespace
{
// This avoids an intermittent shutdown crash deleting a stub-graphics buffer (LP: #1728931)
std::vector<std::shared_ptr<void>> delay_unloading_graphics_platforms;
}

mtf::HeadlessTest::HeadlessTest()
{
    add_to_environment("MIR_SERVER_PLATFORM_PATH", mtf::server_platform_path().c_str());
    add_to_environment("MIR_SERVER_PLATFORM_DISPLAY_LIBS", "mir:stub-graphics");
    add_to_environment("MIR_SERVER_PLATFORM_RENDERING_LIBS", "mir:stub-graphics");
    add_to_environment("MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str());
    add_to_environment("MIR_SERVER_ENABLE_KEY_REPEAT", "false");
    add_to_environment("MIR_SERVER_CONSOLE_PROVIDER", "none");

    server.override_the_display_buffer_compositor_factory(
        [this]() -> std::shared_ptr<mir::compositor::DisplayBufferCompositorFactory>
    {
        auto first_platform = server.the_rendering_platforms().front();
        if (auto gl_provider = mg::RenderingPlatform::acquire_provider<mg::GLRenderingProvider>(std::move(first_platform)))
        {
            return std::make_shared<mtf::HeadlessDisplayBufferCompositorFactory>(std::move(gl_provider), server.the_gl_config());
        }
        BOOST_THROW_EXCEPTION((std::runtime_error{"Test RenderingPlatform does not support GL interface"}));
    });


    server.add_init_callback(
        [server = &server]
        {
            auto display_platforms = server->the_display_platforms();
            auto rendering_platforms = server->the_rendering_platforms();
            delay_unloading_graphics_platforms.insert(
                delay_unloading_graphics_platforms.end(),
                display_platforms.begin(), display_platforms.end());
            delay_unloading_graphics_platforms.insert(
                delay_unloading_graphics_platforms.end(),
                rendering_platforms.begin(), rendering_platforms.end());
        });
}

mtf::HeadlessTest::~HeadlessTest() noexcept
{
    delay_unloading_graphics_platforms.clear();
}

void mtf::HeadlessTest::preset_display(std::unique_ptr<mir::graphics::Display> display)
{
    mtf::set_next_preset_display(std::move(display));
}

void mtf::HeadlessTest::initial_display_layout(std::vector<geom::Rectangle> const& display_rects)
{
    mtf::set_next_display_rects(std::unique_ptr<std::vector<geom::Rectangle>>(new std::vector<geom::Rectangle>(display_rects)));
}
