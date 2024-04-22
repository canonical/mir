/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_CUSTOM_RENDERER_H
#define MIRAL_CUSTOM_RENDERER_H

#include <memory>
#include <functional>

namespace mir
{
class Server;
namespace graphics
{
class GLRenderingProvider;
class GLConfig;
namespace gl
{
class OutputSurface;
}
}
namespace renderer
{
class Renderer;
class RendererFactory;
namespace gl
{
class RenderTarget;
}
}
}

namespace miral
{
typedef std::function<std::unique_ptr<mir::renderer::Renderer>
    (std::unique_ptr<mir::graphics::gl::OutputSurface>,
     std::shared_ptr<mir::graphics::GLRenderingProvider>)> CreateRenderer;

class CustomRenderer
{
public:
    explicit CustomRenderer(CreateRenderer const& renderer, std::shared_ptr<mir::graphics::GLConfig> const&);
    void operator()(mir::Server& server) const;
private:
    std::shared_ptr<mir::renderer::RendererFactory> factory;
    std::shared_ptr<mir::graphics::GLConfig> config;
};
}

#endif //MIRAL_CUSTOM_RENDERER_H
