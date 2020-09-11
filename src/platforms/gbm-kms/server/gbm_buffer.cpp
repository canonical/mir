/*
 * Copyright © 2012 Canonical Ltd.
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
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gbm_buffer.h"
#include "buffer_texture_binder.h"
#include "gbm_format_conversions.h"

#include "mir/graphics/program.h"
#include "mir/graphics/program_factory.h"

#include <fcntl.h>
#include <xf86drm.h>

#include <epoxy/gl.h>
#include <GLES2/gl2ext.h>

#include <boost/throw_exception.hpp>

#include <system_error>

namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace mgc = mir::graphics::common;
namespace geom=mir::geometry;

void mgg::BindResolverTex::bind()
{
    tex_bind();
}

void mgg::BindResolverTexTarget::bind()
{
    upload_to_texture();
}

mgg::GBMBuffer::GBMBuffer(std::shared_ptr<gbm_bo> const& handle,
                          uint32_t /*bo_flags*/,
                          std::unique_ptr<mgc::BufferTextureBinder> texture_binder)
    : gbm_handle{handle},
      texture_binder{std::move(texture_binder)},
      prime_fd{-1}
{
    auto device = gbm_bo_get_device(gbm_handle.get());
    auto gem_handle = gbm_bo_get_handle(gbm_handle.get()).u32;
    auto drm_fd = gbm_device_get_fd(device);

    auto ret = drmPrimeHandleToFD(drm_fd, gem_handle, DRM_CLOEXEC, &prime_fd);

    if (ret)
    {
        std::string const msg("Failed to get PRIME fd from gbm-kms bo");
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), msg}));
    }
}

mgg::GBMBuffer::~GBMBuffer()
{
    if (prime_fd >= 0)
        close(prime_fd);
}

geom::Size mgg::GBMBuffer::size() const
{
    return {gbm_bo_get_width(gbm_handle.get()), gbm_bo_get_height(gbm_handle.get())};
}

geom::Stride mgg::GBMBuffer::stride() const
{
    return geom::Stride(gbm_bo_get_stride(gbm_handle.get()));
}

MirPixelFormat mgg::GBMBuffer::pixel_format() const
{
    return gbm_format_to_mir_format(gbm_bo_get_format(gbm_handle.get()));
}

void mgg::GBMBuffer::gl_bind_to_texture()
{
    texture_binder->gl_bind_to_texture();
}

std::shared_ptr<mg::NativeBuffer> mgg::GBMBuffer::native_buffer_handle() const
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"Native buffer interface removed"}));
}

mg::NativeBufferBase* mgg::GBMBuffer::native_buffer_base()
{
    return this;
}

void mgg::GBMBuffer::secure_for_render()
{
}

void mgg::GBMBuffer::commit()
{
}

void mgg::GBMBuffer::upload_to_texture()
{
    gl_bind_to_texture();
}

void mgg::GBMBuffer::bind_for_write()
{
    upload_to_texture();
}

mg::gl::Program const& mgg::GBMBuffer::shader(
    mg::gl::ProgramFactory& cache) const
{
    static int argb_program{0};

    return cache.compile_fragment_shader(
        &argb_program,
        "",
        "uniform sampler2D tex;\n"
        "vec4 sample_to_rgba(in vec2 texcoord)\n"
        "{\n"
        "    return texture2D(tex, texcoord);\n"
        "}\n");
}

mir::graphics::gl::Texture::Layout mgg::GBMBuffer::layout() const
{
    return Layout::GL;
}

void mgg::GBMBuffer::add_syncpoint()
{

}

void mgg::GBMBuffer::tex_bind()
{
    std::lock_guard<decltype(tex_id_mutex)> lock{tex_id_mutex};
    bool const needs_initialisation = tex_id == 0;
    if (needs_initialisation)
    {
        glGenTextures(1, &tex_id);
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    if (needs_initialisation)
    {
        // The ShmBuffer *should* be immutable, so we can just upload once.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl_bind_to_texture();
    }
}
