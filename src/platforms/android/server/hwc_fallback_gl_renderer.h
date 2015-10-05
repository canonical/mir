/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_HWC_FALLBACK_GL_RENDERER_H_
#define MIR_GRAPHICS_ANDROID_HWC_FALLBACK_GL_RENDERER_H_
#include "mir/geometry/rectangle.h"
#include "mir/geometry/displacement.h"
#include "mir/gl/program.h"
#include "mir/gl/texture_cache.h"
#include "mir/graphics/renderable.h"
#include <memory>

namespace mir
{
namespace gl { class ProgramFactory; }
namespace graphics
{
class GLContext;

namespace android
{
class SwappingGLContext;

class RenderableListCompositor
{
public:
    virtual ~RenderableListCompositor() = default;
    virtual void render(RenderableList const&, geometry::Displacement list_offset, SwappingGLContext const&) const = 0;
protected:
    RenderableListCompositor() = default;
private:
    RenderableListCompositor(RenderableListCompositor const&) = delete;
    RenderableListCompositor& operator=(RenderableListCompositor const&) = delete;
};

class HWCFallbackGLRenderer : public RenderableListCompositor
{
public:
    HWCFallbackGLRenderer(
        gl::ProgramFactory const& program_factory,
        graphics::GLContext const& gl_context,
        geometry::Rectangle const& screen_position);

    void render(RenderableList const&, geometry::Displacement, SwappingGLContext const&) const;
private:
    std::unique_ptr<gl::Program> program;
    std::unique_ptr<gl::TextureCache> texture_cache;

    GLint position_attr;
    GLint texcoord_attr;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_FALLBACK_GL_RENDERER_H_ */
