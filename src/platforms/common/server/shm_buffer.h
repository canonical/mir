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
class EGLContextDelegate;

class ShmBuffer :
    public BufferBasic,
    public NativeBufferBase,
    public graphics::gl::Texture
{
public:
    ~ShmBuffer() noexcept override;

    static bool supports(MirPixelFormat);

    geometry::Size size() const override;
    MirPixelFormat pixel_format() const override;
    NativeBufferBase* native_buffer_base() override;

    void bind() override;
    gl::Program const& shader(gl::ProgramFactory& cache) const override;
    Layout layout() const override;
    void add_syncpoint() override;
protected:
    ShmBuffer(
        geometry::Size const& size,
        MirPixelFormat const& format,
        std::shared_ptr<EGLContextDelegate> egl_delegate);

    /// \note This must be called with a current GL context
    void upload_to_texture(void const* pixels, geometry::Stride const& stride);
private:
    geometry::Size const size_;
    MirPixelFormat const pixel_format_;
    std::shared_ptr<EGLContextDelegate> const egl_delegate;
    GLuint tex_id{0};
};

class MemoryBackedShmBuffer :
    public ShmBuffer,
    public renderer::software::PixelSource
{
public:
    MemoryBackedShmBuffer(
        geometry::Size const& size,
        MirPixelFormat const& pixel_format,
        std::shared_ptr<EGLContextDelegate> egl_delegate);

    void write(unsigned char const* data, size_t size) override;
    void read(std::function<void(unsigned char const*)> const& do_with_pixels) override;
    geometry::Stride stride() const override;

    std::shared_ptr<NativeBuffer> native_buffer_handle() const override;

    void bind() override;

    MemoryBackedShmBuffer(MemoryBackedShmBuffer const&) = delete;
    MemoryBackedShmBuffer& operator=(MemoryBackedShmBuffer const&) = delete;
private:
    geometry::Stride const stride_;
    std::unique_ptr<unsigned char[]> const pixels;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_SHM_BUFFER_H_ */
