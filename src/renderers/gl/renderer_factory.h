/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_RENDERER_GL_RENDERER_FACTORY_H_
#define MIR_RENDERER_GL_RENDERER_FACTORY_H_

#include "mir/renderer/renderer_factory.h"

namespace mir
{
namespace graphics
{
class GLRenderingProvider;
}
namespace renderer
{
namespace gl
{

class RendererFactory : public renderer::RendererFactory
{
public:
    auto create_renderer_for(
        std::unique_ptr<graphics::gl::OutputSurface> output_surface,
        std::shared_ptr<graphics::GLRenderingProvider> gl_provider) const -> std::unique_ptr<renderer::Renderer> override;
};

}
}
}

#endif
