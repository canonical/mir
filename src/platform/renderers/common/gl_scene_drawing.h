/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_RENDERER_COMMON_GL_SCENE_DRAWING_H_
#define MIR_RENDERER_COMMON_GL_SCENE_DRAWING_H_

#include <mir/geometry/rectangle.h>
#include <mir/geometry/size.h>
#include <mir/gl/primitive.h>

#include <GLES2/gl2.h>
#include <glm/glm.hpp>
#include <vector>

namespace mir
{
namespace graphics
{
class Renderable;
namespace gl
{
class ProgramFactory;
class Texture;
}
}

namespace renderer::common
{

/// The frame-global state needed to draw a renderable.
///
/// This is everything `draw()` needs that isn't specific to the individual
/// renderable, and is constant for the duration of a frame.
struct SceneContext
{
    /// The logical area of the scene being drawn
    geometry::Rectangle viewport;
    /// The size, in pixels, of the surface being drawn into
    geometry::Size output_size;
    /// Transform from logical scene coordinates to output orientation
    glm::mat4 display_transform;
    /// Projection from logical scene coordinates to GL clip coordinates
    glm::mat4 screen_to_gl_coords;
    /// Monotonically increasing frame counter.
    ///
    /// Used to avoid re-uploading the scene-global uniforms for every renderable
    /// sharing a program.
    long long frameno;
};

/// Build the projection from logical scene coordinates to GL clip coordinates.
auto screen_to_gl_coords_for(geometry::Rectangle const& viewport) -> glm::mat4;

/// Set the GL viewport, letterboxing the logical viewport into the output.
///
/// Letterboxing moves the glViewport to add black bars in the case that
/// the logical viewport aspect ratio doesn't match the display aspect.
/// This keeps pixels square. Note "black"-bars are really glClearColor.
void set_letterboxed_gl_viewport(
    geometry::Rectangle const& viewport,
    glm::mat4 const& display_transform,
    geometry::Size output_size);

/// Clear the currently bound framebuffer to `colour` (RGBA, 0.0-1.0)
void clear(GLfloat const (&colour)[4]);

/// Define the list of triangles that will be used to render the renderable.
///
/// This generates the 4 vertices of a rectangle covering *renderable*.
///
/// \primitives The list of rendering primitives to be  modified.
/// \param      renderable The renderable surface being tessellated.
void tessellate(std::vector<mir::gl::Primitive>& primitives, graphics::Renderable const& renderable);

/// Draw a single renderable into the currently bound framebuffer.
///
/// \param context     Frame-global drawing state
/// \param renderable  The renderable to draw
/// \param texture     A texture view onto *renderable*'s buffer
/// \param factory     Factory providing the program *texture* is drawn with
/// \param primitives  The primitives *renderable* has been tessellated into
void draw(
    SceneContext const& context,
    graphics::Renderable const& renderable,
    graphics::gl::Texture& texture,
    graphics::gl::ProgramFactory& factory,
    std::vector<mir::gl::Primitive> const& primitives);

}
}

#endif // MIR_RENDERER_COMMON_GL_SCENE_DRAWING_H_
