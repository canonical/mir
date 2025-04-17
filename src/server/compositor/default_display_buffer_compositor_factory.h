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

#ifndef MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_
#define MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_

#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/compositor_report.h"
#include <mir_toolkit/common.h>

namespace mir
{
namespace graphics
{
class RenderingPlatform;
}
namespace renderer
{
class RendererFactory;
}

namespace graphics
{
class GLRenderingProvider;
class GLConfig;
class GraphicBufferAllocator;
}
///  Compositing. Combining renderables into a display image.
namespace compositor
{

class DefaultDisplayBufferCompositorFactory : public DisplayBufferCompositorFactory
{
public:
    DefaultDisplayBufferCompositorFactory(
        std::vector<std::shared_ptr<graphics::GLRenderingProvider>> render_platforms,
        std::shared_ptr<graphics::GLConfig> gl_config,
        std::shared_ptr<renderer::RendererFactory> const& renderer_factory,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<CompositorReport> const& report,
        MirOutputFilter output_filter);

    std::unique_ptr<DisplayBufferCompositor> create_compositor_for(graphics::DisplaySink& display_sink) override;

private:
    std::vector<std::shared_ptr<graphics::GLRenderingProvider>> const platforms;
    std::shared_ptr<graphics::GLConfig> const gl_config;
    std::shared_ptr<renderer::RendererFactory> const renderer_factory;
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<CompositorReport> const report;
    MirOutputFilter output_filter;
};

}
}


#endif /* MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_ */
