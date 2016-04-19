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

#include "screencast_display_buffer.h"
#include "schedule.h"

#include "mir/graphics/buffer.h"
#include "mir/graphics/display.h"
#include "mir/graphics/gl_context.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir/unwind_helpers.h"

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mrgl = mir::renderer::gl;
namespace geom = mir::geometry;

namespace
{
auto as_texture_source(mg::Buffer* buffer)
{
    auto tex = dynamic_cast<mrgl::TextureSource*>(buffer->native_buffer_base());
    if (!tex)
        BOOST_THROW_EXCEPTION(std::logic_error("Buffer does not support GL rendering"));
    return tex;
}
}

mc::ScreencastDisplayBuffer::ScreencastDisplayBuffer(
    geom::Rectangle const& rect,
    geom::Size const& size,
    MirMirrorMode mirror_mode,
    Schedule& free_queue,
    Schedule& ready_queue,
    mg::Display& display)
    : gl_context(display.create_gl_context()),
      rect(rect), mirror_mode_(mirror_mode),
      free_queue(free_queue), ready_queue(ready_queue),
      old_fbo(), old_viewport(),
      color_tex(0), depth_rbo(0), fbo(0)
{
    gl_context->make_current();
    auto unwinder = on_unwind([this] {
        delete_gl_resources();
        gl_context->release_current();
    });

    glGetIntegerv(GL_VIEWPORT, old_viewport);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    /* Set up the color buffer... */
    glGenTextures(1, &color_tex);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* and the depth buffer */
    glGenRenderbuffers(1, &depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          size.width.as_uint32_t(),
                          size.height.as_uint32_t());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create FBO for buffer"));
}

mc::ScreencastDisplayBuffer::~ScreencastDisplayBuffer()
{
    make_current();
    delete_gl_resources();
    release_current();
}

void mc::ScreencastDisplayBuffer::delete_gl_resources()
{
    if (fbo)
        glDeleteFramebuffers(1, &fbo);
    if (depth_rbo)
        glDeleteRenderbuffers(1, &depth_rbo);
    if (color_tex)
        glDeleteTextures(1, &color_tex);
}

geom::Rectangle mc::ScreencastDisplayBuffer::view_area() const
{
    return rect;
}

void mc::ScreencastDisplayBuffer::make_current()
{
    gl_context->make_current();
}

void mc::ScreencastDisplayBuffer::bind()
{
    if (!current_buffer)
        current_buffer = free_queue.next_buffer();

    auto texture_source = as_texture_source(current_buffer.get());
    glBindTexture(GL_TEXTURE_2D, color_tex);
    texture_source->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, color_tex, 0);

    auto const buf_size = current_buffer->size();

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, buf_size.width.as_uint32_t(), buf_size.height.as_uint32_t());
}

void mc::ScreencastDisplayBuffer::release_current()
{
    glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
    glViewport(old_viewport[0], old_viewport[1],
               old_viewport[2], old_viewport[3]);

    gl_context->release_current();
}

bool mc::ScreencastDisplayBuffer::post_renderables_if_optimizable(mg::RenderableList const&)
{
    return false;
}

void mc::ScreencastDisplayBuffer::swap_buffers()
{
    if (current_buffer)
    {

        //TODO: replace this with a fence which the client could use
        //to wait for rendering completion
        glFinish();

        ready_queue.schedule(current_buffer);
        current_buffer = nullptr;
    }
}

MirOrientation mc::ScreencastDisplayBuffer::orientation() const
{
    return mir_orientation_normal;
}

MirMirrorMode mc::ScreencastDisplayBuffer::mirror_mode() const
{
    return mirror_mode_;
}

mg::NativeDisplayBuffer* mc::ScreencastDisplayBuffer::native_display_buffer()
{
    return this;
}
