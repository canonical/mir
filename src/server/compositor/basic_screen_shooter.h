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

#include <mir/compositor/screen_shooter.h>
#include <mir/graphics/platform.h>
#include <mir/renderer/renderer_factory.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/time/clock.h>

#include <mutex>
#include <glm/glm.hpp>

namespace mir
{
class Executor;
namespace renderer
{
class Renderer;
class RendererFactory;
}
namespace graphics
{
class Cursor;
class OutputFilter;
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
        std::shared_ptr<mir::graphics::GLConfig> const& config,
        std::shared_ptr<graphics::OutputFilter> const& output_filter,
        std::shared_ptr<graphics::Cursor> const& cursor);

    void capture(
        std::shared_ptr<renderer::software::WriteMappable> const& buffer,
        geometry::Rectangle const& area,
        glm::mat2 const& transform,
        bool overlay_cursor,
        std::function<void(std::optional<time::Timestamp>)>&& callback) override;

    CompositorID id() const override;

private:
    struct Self
    {
        class OneShotBufferDisplayProvider;
        class OffscreenDisplaySink;

        Self(
            std::shared_ptr<Scene> const& scene,
            std::shared_ptr<time::Clock> const& clock,
            std::shared_ptr<graphics::GLRenderingProvider> provider,
            std::shared_ptr<renderer::RendererFactory> render_factory,
            std::shared_ptr<mir::graphics::GLConfig> const& config,
            std::shared_ptr<graphics::OutputFilter> const& output_filter,
std::shared_ptr<graphics::Cursor> const& cursor);

        auto render(
            std::shared_ptr<renderer::software::WriteMappable> const& buffer,
            geometry::Rectangle const& area,
            glm::mat2 const& transform,
            bool overlay_cursor) -> time::Timestamp;

        auto renderer_for(geometry::Size buffer_size, MirPixelFormat buffer_format)
            -> renderer::Renderer&;

        std::mutex mutex;
        std::shared_ptr<Scene> const scene;
        std::shared_ptr<time::Clock> const clock;
        std::shared_ptr<graphics::GLRenderingProvider> const render_provider;
        std::shared_ptr<renderer::RendererFactory> const renderer_factory;

        /* The Renderer resizes its output surface to match whatever buffer we're
         * capturing into, so a single instance serves captures of any size.
         * Only a change of pixel format requires building a new one.
         */
        std::unique_ptr<renderer::Renderer> current_renderer;
        MirPixelFormat last_rendered_format;

        std::unique_ptr<OffscreenDisplaySink> offscreen_sink;
        std::shared_ptr<OneShotBufferDisplayProvider> const output;
        std::shared_ptr<mir::graphics::GLConfig> config;
        std::shared_ptr<graphics::OutputFilter> const output_filter;
        std::shared_ptr<graphics::Cursor> cursor;
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
