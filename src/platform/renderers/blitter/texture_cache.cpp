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

#define MIR_LOG_COMPONENT "BlitterRenderer"

#include "texture_cache.h"
#include "software_egl_context.h"

#include <mir/graphics/buffer.h>
#include <mir/graphics/gl_format.h>
#include <mir/graphics/program_factory.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/log.h>

#include <GLES2/gl2ext.h>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace mg = mir::graphics;
namespace mrb = mir::renderer::blitter;

namespace
{
auto make_texture() -> mir::renderer::common::TextureHandle
{
    GLuint tex{0};
    glGenTextures(1, &tex);
    return mir::renderer::common::TextureHandle{tex};
}
}

mrb::TextureCache::TextureCache(
    std::shared_ptr<SoftwareEGLContext> context,
    std::shared_ptr<mg::BlitterRenderingProvider> blitter)
    : context{std::move(context)},
      blitter{std::move(blitter)}
{
}

mrb::TextureCache::~TextureCache()
{
    if (textures.empty())
    {
        return;
    }

    auto const restore_egl_state = preserve_egl_state_unless_current(*context);
    try
    {
        context->make_current();
    }
    catch (...)
    {
        // Deleting with some other context current could delete *its* textures,
        // so prefer to leak ours by having no context current at all.
        mir::log_warning("Failed to make EGL context current to release blitter fallback textures; leaking them");
        try
        {
            context->release_current();
        }
        catch (...)
        {
            // Some other context may still be current, so we can't even let the GL handles be destroyed
            mir::log_warning("Failed to release EGL context; leaking blitter fallback texture objects");
            for (auto& [_, entry] : textures)
            {
                std::ignore = entry.texture.release();
            }
        }
    }

    textures.clear();
}

auto mrb::TextureCache::get(mg::Buffer const& buffer) -> mg::gl::Texture&
{
    context->make_current();

    auto& entry = textures[buffer.id()];
    if (!entry.texture)
    {
        entry.texture = std::make_unique<MappedBufferTexture>();
    }
    entry.used_this_frame = true;

    // We have no way of knowing whether the client has re-submitted this buffer
    // with new content, so we have to assume it has.
    entry.texture->upload(buffer);

    return *entry.texture;
}

void mrb::TextureCache::drop_unused()
{
    auto const unused = [](auto const& entry) { return !entry.second.used_this_frame; };

    // This is called every frame, so avoid switching contexts when there's nothing to drop
    if (std::ranges::any_of(textures, unused))
    {
        auto const restore_egl_state = preserve_egl_state_unless_current(*context);
        context->make_current();
        std::erase_if(textures, unused);
    }

    for (auto& [_, entry] : textures)
    {
        entry.used_this_frame = false;
    }
}

void mrb::TextureCache::invalidate()
{
    if (textures.empty())
    {
        return;
    }

    auto const restore_egl_state = preserve_egl_state_unless_current(*context);
    context->make_current();
    textures.clear();
}

mrb::TextureCache::MappedBufferTexture::MappedBufferTexture()
    : texture{make_texture()}
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

auto mrb::TextureCache::MappedBufferTexture::shader(mg::gl::ProgramFactory& factory) const -> mg::gl::Program const&
{
    static int argb_shader{0};
    return factory.compile_fragment_shader(
        &argb_shader,
        "",
        "uniform sampler2D tex;\n"
        "vec4 sample_to_rgba(in vec2 texcoord)\n"
        "{\n"
        "    return texture2D(tex, texcoord);\n"
        "}\n");
}

auto mrb::TextureCache::MappedBufferTexture::layout() const -> Layout
{
    return Layout::GL;
}

void mrb::TextureCache::MappedBufferTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, texture);
}

auto mrb::TextureCache::MappedBufferTexture::tex_id() const -> GLuint
{
    return texture;
}

void mrb::TextureCache::MappedBufferTexture::add_syncpoint()
{
}

void mrb::TextureCache::MappedBufferTexture::upload(mg::Buffer const& buffer)
{
    auto const mapping = buffer.map_readable();
    if (!mapping)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Blitter provider failed to map a buffer for CPU access"}));
    }

    GLenum format{0};
    GLenum type{0};
    if (!mg::get_gl_pixel_format(mapping->format(), format, type))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "Buffer has a pixel format with no GL equivalent; cannot draw it without the blitter"}));
    }

    auto const size = mapping->size();

    // As elsewhere in Mir we assume the stride is a whole number of pixels, which is technically
    // not always true, but no client has yet been observed doing otherwise.
    auto const stride_in_px = mapping->stride().as_int() / MIR_BYTES_PER_PIXEL(mapping->format());

    GLint previous_row_length{0};
    GLint previous_alignment{4};
    glGetIntegerv(GL_UNPACK_ROW_LENGTH_EXT, &previous_row_length);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_alignment);

    bind();
    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride_in_px);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, 0, static_cast<GLint>(format),
        size.width.as_int(), size.height.as_int(),
        0, format, type, mapping->data());

    // Be nice to other users of the GL context by reverting our changes to shared state
    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, previous_row_length);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previous_alignment);
}
