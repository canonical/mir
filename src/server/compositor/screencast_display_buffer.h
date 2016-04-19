/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_
#define MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/render_target.h"

#include <GLES2/gl2.h>

namespace mir
{
namespace graphics
{
class Display;
class GLContext;
}
namespace renderer { namespace gl { class TextureSource; }}
namespace compositor
{
class Schedule;
class ScreencastDisplayBuffer : public graphics::DisplayBuffer,
                                public graphics::NativeDisplayBuffer,
                                public renderer::gl::RenderTarget
{
public:
    ScreencastDisplayBuffer(
        geometry::Rectangle const& rect,
        geometry::Size const& size,
        MirMirrorMode mirror_mode,
        Schedule& free_queue,
        Schedule& ready_queue,
        graphics::Display& display);
    ~ScreencastDisplayBuffer();

    geometry::Rectangle view_area() const override;

    void make_current() override;

    void bind() override;

    void release_current() override;

    bool post_renderables_if_optimizable(graphics::RenderableList const&) override;

    void swap_buffers() override;

    MirOrientation orientation() const override;

    MirMirrorMode mirror_mode() const override;

    NativeDisplayBuffer* native_display_buffer() override;

private:
    void delete_gl_resources();
    std::unique_ptr<graphics::GLContext> gl_context;
    geometry::Rectangle const rect;
    MirMirrorMode const mirror_mode_;

    Schedule& free_queue;
    Schedule& ready_queue;
    std::shared_ptr<graphics::Buffer> current_buffer;

    GLint old_fbo;
    GLint old_viewport[4];

    GLuint color_tex;
    GLuint depth_rbo;
    GLuint fbo;
};

}
}

#endif /* MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_ */
