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

#include <mir/graphics/buffer.h>
#include <mir/graphics/gl_format.h>
#include <mir/graphics/program_factory.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/log.h>

#include <GLES2/gl2ext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>
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

mrb::MappedBufferTexture::MappedBufferTexture()
    : texture{make_texture()}
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

auto mrb::MappedBufferTexture::shader(mg::gl::ProgramFactory& factory) const -> mg::gl::Program const&
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

auto mrb::MappedBufferTexture::layout() const -> Layout
{
    return Layout::GL;
}

void mrb::MappedBufferTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, texture);
}

auto mrb::MappedBufferTexture::tex_id() const -> GLuint
{
    return texture;
}

void mrb::MappedBufferTexture::add_syncpoint()
{
}

void mrb::MappedBufferTexture::upload(mg::BlitterRenderingProvider& blitter, mg::Buffer const& buffer)
{
    auto const mapping = blitter.map_buffer(buffer);
    if (!mapping)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Blitter provider failed to map a buffer for CPU access"}));
    }

    GLenum format, type;
    if (!mg::get_gl_pixel_format(mapping->format(), format, type))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "Buffer has a pixel format with no GL equivalent; cannot draw it without the blitter"}));
    }

    auto const size = mapping->size();
    /* As elsewhere in Mir we assume the stride is a whole number of pixels;
     * it need not be, but no client has yet been observed doing otherwise.
     */
    auto const stride_in_px = mapping->stride().as_int() / MIR_BYTES_PER_PIXEL(mapping->format());

    bind();
    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride_in_px);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, 0, format,
        size.width.as_int(), size.height.as_int(),
        0, format, type, mapping->data());

    // Be nice to other users of the GL context by reverting our changes to shared state
    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

mrb::TextureCache::TextureCache(std::shared_ptr<mg::BlitterRenderingProvider> blitter)
    : blitter{std::move(blitter)}
{
}

auto mrb::TextureCache::get(mg::Buffer const& buffer) -> mg::gl::Texture&
{
    auto& entry = textures[buffer.id()];
    if (!entry.texture)
    {
        entry.texture = std::make_unique<MappedBufferTexture>();
    }
    entry.used_this_frame = true;

    /* We have no way of knowing whether the client has re-submitted this buffer
     * with new content, so we have to assume it has.
     */
    entry.texture->upload(*blitter, buffer);

    return *entry.texture;
}

void mrb::TextureCache::drop_unused()
{
    std::erase_if(textures, [](auto const& entry) { return !entry.second.used_this_frame; });
    for (auto& [_, entry] : textures)
    {
        entry.used_this_frame = false;
    }
}

void mrb::TextureCache::invalidate()
{
    textures.clear();
}
