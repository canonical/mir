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

#ifndef MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_H_
#define MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_H_

#include "mir/compositor/display_buffer_compositor.h"
#include "mir/graphics/platform.h"
#include <memory>

namespace mir
{
namespace compositor
{
class CompositorReport;
}
namespace graphics
{
class DisplaySink;
class OutputFilter;
}
namespace renderer
{
class Renderer;
}
namespace compositor
{

class Scene;

class DefaultDisplayBufferCompositor : public DisplayBufferCompositor
{
public:
    DefaultDisplayBufferCompositor(
        graphics::DisplaySink& display_sink,
        graphics::GLRenderingProvider& gl_provider,
        std::shared_ptr<renderer::Renderer> const& renderer,
        std::shared_ptr<graphics::OutputFilter> const& output_filter,
        std::shared_ptr<compositor::CompositorReport> const& report);

    bool composite(SceneElementSequence&& scene_sequence) override;

private:
    graphics::DisplaySink& display_sink;
    std::shared_ptr<renderer::Renderer> const renderer;
    std::shared_ptr<graphics::OutputFilter> const output_filter;
    std::unique_ptr<graphics::RenderingProvider::FramebufferProvider> const fb_adaptor;
    std::shared_ptr<compositor::CompositorReport> const report;
    bool completed_first_render = false;
};

}
}

#endif /* MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_H_ */
