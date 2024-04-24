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

#include "miral/custom_renderer.h"
#include <mir/server.h>
#include <mir/renderer/renderer_factory.h>
#include <mir/renderer/renderer.h>
#include <mir/renderer/gl/gl_surface.h>

struct miral::CustomRenderer::Self : public mir::renderer::RendererFactory
{
    explicit Self(Builder&& renderer)
        : renderer(std::move(renderer))
    {
    }
    [[nodiscard]] auto create_renderer_for(
        std::unique_ptr<mir::graphics::gl::OutputSurface> output_surface,
        std::shared_ptr<mir::graphics::GLRenderingProvider> gl_provider) const
        -> std::unique_ptr<mir::renderer::Renderer> override
    {
        return renderer(std::move(output_surface), std::move(gl_provider));
    }
private:
    miral::CustomRenderer::Builder const renderer;
};

miral::CustomRenderer::CustomRenderer(Builder&& renderer)
    : self{std::make_shared<Self>(std::move(renderer))}
{
}

void miral::CustomRenderer::operator()(mir::Server &server) const
{
    std::function<std::shared_ptr<mir::renderer::RendererFactory>()> builder = [&]() { return self; };
    server.override_the_renderer_factory(builder);
}