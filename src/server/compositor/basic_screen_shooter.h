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

#ifndef MIR_COMPOSITOR_BASIC_SCREEN_SHOOTER_H_
#define MIR_COMPOSITOR_BASIC_SCREEN_SHOOTER_H_

#include "mir/compositor/screen_shooter.h"
#include "mir/graphics/platform.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/time/clock.h"

#include <mutex>

namespace mir
{
class Executor;
namespace renderer
{
class Renderer;
class RendererFactory;
}
namespace compositor
{
class Scene;

class BasicScreenShooter: public ScreenShooter
{
public:
    BasicScreenShooter(
        std::shared_ptr<Scene> const& scene,
        std::shared_ptr<time::Clock> const& clock,
        Executor& executor,
        std::span<std::shared_ptr<graphics::GLRenderingProvider>> const& providers,
        std::shared_ptr<renderer::RendererFactory> render_factory,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<mir::graphics::GLConfig> const& config);

    void capture(
        std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
        geometry::Rectangle const& area,
        std::function<void(std::optional<time::Timestamp>)>&& callback) override;

private:
    struct Self
    {
        class OneShotBufferDisplayProvider;

        Self(
            std::shared_ptr<Scene> const& scene,
            std::shared_ptr<time::Clock> const& clock,
            std::shared_ptr<graphics::GLRenderingProvider> provider,
            std::shared_ptr<renderer::RendererFactory> render_factory,
            std::shared_ptr<mir::graphics::GLConfig> const& config);

        auto render(
            std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
            geometry::Rectangle const& area) -> time::Timestamp;

        auto renderer_for_buffer(std::shared_ptr<renderer::software::WriteMappableBuffer> buffer)
            -> renderer::Renderer&;

        std::mutex mutex;
        std::shared_ptr<Scene> const scene;
        std::shared_ptr<time::Clock> const clock;
        std::shared_ptr<graphics::GLRenderingProvider> const render_provider;
        std::shared_ptr<renderer::RendererFactory> const renderer_factory;

        /* The Renderer instantiation is tied to a particular output size, and
         * requires enough setup to make it worth keeping around as a consumer
         * is likely to be taking screenshots of consistent size
         */
        std::unique_ptr<renderer::Renderer> current_renderer;
        geometry::Size last_rendered_size;

        std::unique_ptr<graphics::DisplaySink> offscreen_sink;
        std::shared_ptr<OneShotBufferDisplayProvider> const output;
        std::shared_ptr<mir::graphics::GLConfig> config;
    };
    std::shared_ptr<Self> const self;
    Executor& executor;

    static auto select_provider(
        std::span<std::shared_ptr<graphics::GLRenderingProvider>> const& providers,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator)
        -> std::shared_ptr<graphics::GLRenderingProvider>;
};
}
}

#endif // MIR_COMPOSITOR_BASIC_SCREEN_SHOOTER_H_
