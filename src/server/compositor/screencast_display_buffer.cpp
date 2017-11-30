/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "screencast_display_buffer.h"
#include "schedule.h"

#include "mir/graphics/buffer.h"
#include "mir/graphics/display.h"
#include "mir/graphics/transformation.h"
#include "mir/renderer/gl/context.h"
#include "mir/renderer/gl/texture_target.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/raii.h"

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mrgl = mir::renderer::gl;
namespace geom = mir::geometry;

namespace
{
auto as_texture_target(mg::Buffer* buffer)
{
    auto tex = dynamic_cast<mrgl::TextureTarget*>(buffer->native_buffer_base());
    if (!tex)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Buffer does not support being rendered to as a GL target"));
    return tex;
}

auto as_context_source(mg::Display* display)
{
    auto const ctx = dynamic_cast<mrgl::ContextSource*>(display->native_display());
    if (!ctx)
        BOOST_THROW_EXCEPTION(std::logic_error("Display does not support GL rendering"));
    return ctx;
}

template <void (*Generate)(GLsizei,GLuint*), void (*Delete)(GLsizei,GLuint const*)>
mc::detail::GLResource<Delete> allocate_gl_resource()
{
    GLuint resource{0};
    Generate(1, &resource);
    return mc::detail::GLResource<Delete>{resource};
}
}

mc::ScreencastDisplayBuffer::ScreencastDisplayBuffer(
    geom::Rectangle const& rect,
    geom::Size const& size,
    MirMirrorMode mirror_mode,
    Schedule& free_queue,
    Schedule& ready_queue,
    mg::Display& display)
    : gl_context(as_context_source(&display)->create_gl_context()),
      rect(rect),
      transform(mg::transformation(mirror_mode)),
      free_queue(free_queue), ready_queue(ready_queue),
      old_fbo(), old_viewport(),
      current_size(size)
{
    auto const gl_context_raii = mir::raii::paired_calls(
        [this] { gl_context->make_current(); },
        [this] { gl_context->release_current(); });

    glGetIntegerv(GL_VIEWPORT, old_viewport);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);

    auto texture = allocate_gl_resource<glGenTextures, glDeleteTextures>();
    auto depth_buffer = allocate_gl_resource<glGenRenderbuffers, glDeleteRenderbuffers>();
    auto framebuffer = allocate_gl_resource<glGenFramebuffers, glDeleteFramebuffers>();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    /* Set up the color buffer... */
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* and the depth buffer */
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          size.width.as_uint32_t(),
                          size.height.as_uint32_t());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_buffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create FBO for buffer"));

    color_tex = std::move(texture);
    depth_rbo =  std::move(depth_buffer);
    fbo =  std::move(framebuffer);
}

mc::ScreencastDisplayBuffer::~ScreencastDisplayBuffer()
{
    make_current();
    color_tex.reset();
    depth_rbo.reset();
    fbo.reset();
    release_current();
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

    auto texture_target = as_texture_target(current_buffer.get());
    glBindTexture(GL_TEXTURE_2D, color_tex);
    texture_target->bind_for_write();
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

bool mc::ScreencastDisplayBuffer::overlay(mg::RenderableList const&)
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


        commit();

        ready_queue.schedule(current_buffer);
        current_buffer = nullptr;
    }
}

void mc::ScreencastDisplayBuffer::commit()
{
    if (auto const texture_target = as_texture_target(current_buffer.get()))
    {
        texture_target->commit();
    }
}

glm::mat2 mc::ScreencastDisplayBuffer::transformation() const
{
    return transform;
}

mg::NativeDisplayBuffer* mc::ScreencastDisplayBuffer::native_display_buffer()
{
    return this;
}

geom::Size mc::ScreencastDisplayBuffer::renderbuffer_size()
{
    return current_size;
}

void mc::ScreencastDisplayBuffer::set_renderbuffer_size(geom::Size size)
{
    if (size == current_size)
        return;

    auto depth_buffer = allocate_gl_resource<glGenRenderbuffers, glDeleteRenderbuffers>();
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          size.width.as_uint32_t(),
                          size.height.as_uint32_t());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_buffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create FBO for buffer"));
    depth_rbo =  std::move(depth_buffer);
}

void mc::ScreencastDisplayBuffer::set_transformation(glm::mat2 const& t)
{
    transform = t;
}
