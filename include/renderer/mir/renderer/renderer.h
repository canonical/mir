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

#ifndef MIR_RENDERER_RENDERER_H_
#define MIR_RENDERER_RENDERER_H_

#include "mir/geometry/rectangle.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/output_filter.h"
#include "mir_toolkit/common.h"
#include <glm/glm.hpp>

namespace mir
{
namespace graphics
{
class Framebuffer;
class OutputFilter;
}

namespace renderer
{

class Renderer
{
public:
    virtual ~Renderer() = default;

    virtual void set_viewport(geometry::Rectangle const& rect) = 0;
    virtual void set_output_transform(glm::mat2 const&) = 0;
    virtual void set_output_filter(std::shared_ptr<graphics::OutputFilter> const& filter) = 0;
    virtual auto render(graphics::RenderableList const&) const -> std::unique_ptr<graphics::Framebuffer> = 0;
    virtual void suspend() = 0; // called when render() is skipped

protected:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
};

}
}

#endif // MIR_RENDERER_RENDERER_H_
