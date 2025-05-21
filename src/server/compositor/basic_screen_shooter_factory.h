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

#ifndef MIR_COMPOSITOR_BASIC_SCREEN_SHOOTER_FACTORY_H
#define MIR_COMPOSITOR_BASIC_SCREEN_SHOOTER_FACTORY_H

#include "mir/compositor/screen_shooter_factory.h"
#include "mir/graphics/platform.h"

namespace mir
{
class Executor;
namespace renderer
{
class Renderer;
class RendererFactory;
}

namespace time
{
class Clock;
}

namespace compositor
{
class Scene;

class BasicScreenShooterFactory : public ScreenShooterFactory
{
public:
    BasicScreenShooterFactory(
        std::shared_ptr<Scene> const& scene,
        std::shared_ptr<time::Clock> const& clock,
        Executor& executor,
        std::vector<std::shared_ptr<graphics::GLRenderingProvider>> const& providers,
        std::shared_ptr<renderer::RendererFactory> const& render_factory,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<graphics::GLConfig> const& config);
    auto create() -> std::unique_ptr<ScreenShooter> override;

private:
    std::shared_ptr<Scene> const scene;
    std::shared_ptr<time::Clock> const clock;
    Executor& executor;
    std::vector<std::shared_ptr<graphics::GLRenderingProvider>> providers;
    std::shared_ptr<renderer::RendererFactory> const renderer_factory;
    std::shared_ptr<graphics::GraphicBufferAllocator> buffer_allocator;
    std::shared_ptr<graphics::GLConfig> config;
};
}
}

#endif //MIR_COMPOSITOR_BASIC_SCREEN_SHOOTER_FACTORY_H
