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

#include "mir/default_server_configuration.h"

#include "mir/log.h"
#include "mir/shell/shell.h"
#include "default_display_buffer_compositor_factory.h"
#include "mir/executor.h"
#include "multi_threaded_compositor.h"
#include "gl/renderer_factory.h"
#include "basic_screen_shooter.h"
#include "basic_screen_shooter_factory.h"
#include "null_screen_shooter.h"
#include "null_screen_shooter_factory.h"
#include "mir/main_loop.h"
#include "mir/graphics/platform.h"
#include "mir/options/configuration.h"

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace mg = mir::graphics;

std::shared_ptr<mc::DisplayBufferCompositorFactory>
mir::DefaultServerConfiguration::the_display_buffer_compositor_factory()
{
    return display_buffer_compositor_factory(
        [this]()
        {
            std::vector<std::shared_ptr<mg::GLRenderingProvider>> providers;
            providers.reserve(the_rendering_platforms().size());
            for (auto const& platform : the_rendering_platforms())
            {
                if (auto gl_provider = mg::RenderingPlatform::acquire_provider<mg::GLRenderingProvider>(platform))
                {
                    providers.push_back(gl_provider);            
                }
            }
            if (providers.empty())
            {
                BOOST_THROW_EXCEPTION((std::runtime_error{"Selected rendering platform does not support GL"}));
            }
            return wrap_display_buffer_compositor_factory(
                std::make_shared<mc::DefaultDisplayBufferCompositorFactory>(
                    std::move(providers), the_gl_config(), the_renderer_factory(), the_buffer_allocator(), the_compositor_report()));
        });
}

std::shared_ptr<mc::DisplayBufferCompositorFactory>
mir::DefaultServerConfiguration::wrap_display_buffer_compositor_factory(
    std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped)
{
    return wrapped;
}

std::shared_ptr<mc::Compositor>
mir::DefaultServerConfiguration::the_compositor()
{
    return compositor(
        [this]()
        {
            std::chrono::milliseconds const composite_delay(
                the_options()->get<int>(options::composite_delay_opt));

            return std::make_shared<mc::MultiThreadedCompositor>(
                the_display(),
                the_display_buffer_compositor_factory(),
                the_scene(),
                the_shell(),
                the_compositor_report(),
                composite_delay,
                true);
        });
}

std::shared_ptr<mir::renderer::RendererFactory> mir::DefaultServerConfiguration::the_renderer_factory()
{
    return renderer_factory(
        []()
        {
            return std::make_shared<mir::renderer::gl::RendererFactory>();
        });
}

auto mir::DefaultServerConfiguration::the_screen_shooter() -> std::shared_ptr<compositor::ScreenShooter>
{
    return screen_shooter(
        [this]() -> std::shared_ptr<compositor::ScreenShooter>
        {
            try
            {
                std::vector<std::shared_ptr<mg::GLRenderingProvider>> providers;
                providers.reserve(the_rendering_platforms().size());
                for (auto& platform : the_rendering_platforms())
                {
                    if (auto gl_provider = mg::RenderingPlatform::acquire_provider<mg::GLRenderingProvider>(platform))
                    {
                        providers.push_back(gl_provider);
                    }
                }
                if (providers.empty())
                {
                    BOOST_THROW_EXCEPTION((std::runtime_error{"No platform provides GL rendering support"}));
                }

                return std::make_shared<compositor::BasicScreenShooter>(
                    the_scene(),
                    the_clock(),
                    thread_pool_executor,
                    providers,
                    the_renderer_factory(),
                    the_buffer_allocator(),
                    the_gl_config());
            }
            catch (...)
            {
                mir::log(
                    ::mir::logging::Severity::error,
                    "",
                    std::current_exception(),
                    "failed to create BasicScreenShooter");
                return std::make_shared<compositor::NullScreenShooter>(thread_pool_executor);
            }
        });
}

auto mir::DefaultServerConfiguration::the_screen_shooter_factory() -> std::shared_ptr<compositor::ScreenShooterFactory>
{
    return screen_shooter_factory(
        [this]() ->std::shared_ptr<compositor::ScreenShooterFactory>
        {
            std::vector<std::shared_ptr<mg::GLRenderingProvider>> providers;
            providers.reserve(the_rendering_platforms().size());
            for (auto& platform : the_rendering_platforms())
            {
                if (auto gl_provider = mg::RenderingPlatform::acquire_provider<mg::GLRenderingProvider>(platform))
                {
                    providers.push_back(gl_provider);
                }
            }
            if (providers.empty())
            {
                log_error("Failed to create screen shooter factory: No platform provides GL rendering support");
                return std::make_shared<compositor::NullScreenShooterFactory>(thread_pool_executor);
            }

            return std::make_shared<compositor::BasicScreenShooterFactory>(
                the_scene(),
                the_clock(),
                thread_pool_executor,
                providers,
                the_renderer_factory(),
                the_buffer_allocator(),
                the_gl_config());
        });
}
