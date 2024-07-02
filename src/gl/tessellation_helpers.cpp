/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "mir/gl/tessellation_helpers.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"

namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace geom = mir::geometry;

namespace
{
struct SrcTexCoords
{
    GLfloat top;
    GLfloat bottom;
    GLfloat left;
    GLfloat right;
};

auto tex_coords_from_rect(geom::Size buffer_size, geom::RectangleD sample_rect) -> SrcTexCoords
{
    /* GL Texture coordinates are normalised to the size of the buffer, so (0.0, 0.0) is the top-left
     * and (1.0, 1.0) is the bottom-right
     */
    SrcTexCoords coords;
    coords.top = sample_rect.top().as_value() / buffer_size.height.as_uint32_t();
    coords.bottom = sample_rect.bottom().as_value() / buffer_size.height.as_uint32_t();
    coords.left = sample_rect.left().as_value() / buffer_size.width.as_uint32_t();
    coords.right = sample_rect.right().as_value() / buffer_size.width.as_uint32_t();
    return coords;
}
}

mgl::Primitive mgl::tessellate_renderable_into_rectangle(
    mg::Renderable const& renderable, geom::Displacement const& offset)
{
    auto rect = renderable.screen_position();
    rect.top_left = rect.top_left - offset;
    GLfloat const left = rect.top_left.x.as_int();
    GLfloat const right = left + rect.size.width.as_int();
    GLfloat const top = rect.top_left.y.as_int();
    GLfloat const bottom = top + rect.size.height.as_int();

    mgl::Primitive rectangle;
    rectangle.type = GL_TRIANGLE_STRIP;

    auto const [tex_top, tex_bottom, tex_left, tex_right] =
        tex_coords_from_rect(renderable.buffer()->size(), renderable.src_bounds());

    auto& vertices = rectangle.vertices;
    vertices[0] = {{left,  top,    0.0f}, {tex_left, tex_top}};
    vertices[1] = {{left,  bottom, 0.0f}, {tex_left, tex_bottom}};
    vertices[2] = {{right, top,    0.0f}, {tex_right, tex_top}};
    vertices[3] = {{right, bottom, 0.0f}, {tex_right, tex_bottom}};
    return rectangle;
}
