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

#include "renderer_factory.h"
#include "renderer.h"
#include "mir/graphics/platform.h"
#include "mir/renderer/gl/gl_surface.h"

namespace mrg = mir::renderer::gl;

auto mrg::RendererFactory::create_renderer_for(
    std::unique_ptr<graphics::gl::OutputSurface> output_surface,
    std::shared_ptr<graphics::GLRenderingProvider> gl_provider) const -> std::unique_ptr<mir::renderer::Renderer>
{
    return std::make_unique<Renderer>(std::move(gl_provider), std::move(output_surface));
}
