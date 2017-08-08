/*
 * Copyright © 2015, 2016 Canonical Ltd.
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
 *
 * Authored By: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_EXAMPLES_ADORNING_COMPOSITOR_H_
#define MIR_EXAMPLES_ADORNING_COMPOSITOR_H_
#include "mir/compositor/display_buffer_compositor.h"
#include <GLES2/gl2.h>
#include <tuple>

namespace mir
{
namespace graphics
{
class DisplayBuffer;
}
namespace renderer { namespace gl { class RenderTarget; }}
namespace compositor { class CompositorReport; }

namespace examples
{
class AdorningDisplayBufferCompositor : public compositor::DisplayBufferCompositor
{
public:
    AdorningDisplayBufferCompositor(
        graphics::DisplayBuffer&, std::tuple<float, float, float> const& background_rgb,
        std::shared_ptr<compositor::CompositorReport> const& report);

    void composite(compositor::SceneElementSequence&& scene_sequence) override;
private:
    graphics::DisplayBuffer& db;
    renderer::gl::RenderTarget* const render_target;
    GLchar const*const vert_shader_src;
    GLchar const*const frag_shader_src;
    bool current;
    struct Shader
    {
        Shader(GLchar const* const* src, GLuint type);
        ~Shader();
        GLuint shader;
    } vertex, fragment;
    struct Program
    {
        Program(Shader& vertex, Shader& fragment);
        ~Program();
        GLuint program;
    } program;

    GLfloat vertex_data[16]
    {
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
    };
    GLfloat uv_data[8]
    {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,

    };

    GLuint vPositionAttr;
    GLuint uvCoord;
    GLuint scaleUniform;
    GLuint posUniform;
    GLuint alphaUniform; 
    GLuint texture;

    std::shared_ptr<compositor::CompositorReport> const report;
};
}
}

#endif /* MIR_EXAMPLES_ADORNING_COMPOSITOR_H_ */
