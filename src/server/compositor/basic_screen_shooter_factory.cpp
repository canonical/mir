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

#include "basic_screen_shooter_factory.h"
#include "basic_screen_shooter.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mr = mir::renderer;
namespace mt = mir::time;

mc::BasicScreenShooterFactory::BasicScreenShooterFactory(
    std::shared_ptr<Scene> const& scene,
    std::shared_ptr<mt::Clock> const& clock,
    std::vector<std::shared_ptr<mg::GLRenderingProvider>> const& providers,
    std::shared_ptr<mr::RendererFactory> const& render_factory,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mg::GLConfig> const& config,
    std::shared_ptr<mg::OutputFilter> const& output_filter,
    std::shared_ptr<graphics::Cursor> const& cursor)
    : scene(scene),
      clock(clock),
      providers(providers),
      renderer_factory(render_factory),
      buffer_allocator(buffer_allocator),
      config(config),
      output_filter(output_filter),
      cursor(cursor)
{}

auto mc::BasicScreenShooterFactory::create(Executor& executor) -> std::unique_ptr<ScreenShooter>
{
    return std::make_unique<BasicScreenShooter>(
        scene, clock, executor, providers, renderer_factory, buffer_allocator, config, output_filter, cursor);
}
