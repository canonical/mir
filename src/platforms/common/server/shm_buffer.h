/*
 * Copyright © 2013 Canonical Ltd.
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
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_SHM_BUFFER_H_
#define MIR_GRAPHICS_MESA_SHM_BUFFER_H_

#include "mir/graphics/buffer_basic.h"
#include "mir/geometry/dimensions.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir/renderer/gl/texture_target.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/texture.h"

#include MIR_SERVER_GL_H

namespace mir
{
class ShmFile;

namespace graphics
{
namespace common
{
/*
 * renderer::gl::TextureSource and graphics::gl::Texture both have
 * a bind() method. They need to do different things.
 *
 * Because we can't just override them based on their signature,
 * do the intermediate-base-class trick of having two proxy bases
 * which do nothing but rename bind() to something unique.
 */

class BindResolverTex : public gl::Texture
{
public:
    BindResolverTex() = default;

    void bind() override final
    {
        tex_bind();
    }

protected:
    virtual void tex_bind() = 0;
};

class BindResolverTexTarget : public renderer::gl::TextureSource
{
public:
    BindResolverTexTarget() = default;

    void bind() override final
    {
        upload_to_texture();
    }

protected:
    virtual void upload_to_texture() = 0;
};

class ShmBuffer : public BufferBasic, public NativeBufferBase,
                  public BindResolverTexTarget,
                  public renderer::gl::TextureTarget,
                  public renderer::software::PixelSource,
                  public BindResolverTex
{
public:
    static bool supports(MirPixelFormat);

    ~ShmBuffer() noexcept;

    geometry::Size size() const override;
    geometry::Stride stride() const override;
    MirPixelFormat pixel_format() const override;
    void gl_bind_to_texture() override;
    void upload_to_texture() override;
    void secure_for_render() override;
    void write(unsigned char const* data, size_t size) override;
    void read(std::function<void(unsigned char const*)> const& do_with_pixels) override;
    NativeBufferBase* native_buffer_base() override;

    void tex_bind() override;
    gl::Program const& shader(gl::ProgramFactory& cache) const override;
    Layout layout() const override;
    void add_syncpoint() override;

    //each platform will have to return the NativeBuffer type that the platform has defined.
    virtual std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const override = 0;

    void bind_for_write() override;
    void commit() override;

protected:
    ShmBuffer(std::unique_ptr<ShmFile> shm_file,
              geometry::Size const& size,
              MirPixelFormat const& pixel_format);

    std::shared_ptr<MirBufferPackage> to_mir_buffer_package() const;

private:
    ShmBuffer(ShmBuffer const&) = delete;
    ShmBuffer& operator=(ShmBuffer const&) = delete;

    std::unique_ptr<ShmFile> const shm_file;
    geometry::Size const size_;
    MirPixelFormat const pixel_format_;
    geometry::Stride const stride_;
    void* const pixels;
    GLuint tex_id{0};
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_SHM_BUFFER_H_ */
