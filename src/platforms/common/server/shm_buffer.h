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
 */

#ifndef MIR_GRAPHICS_GBM_SHM_BUFFER_H_
#define MIR_GRAPHICS_GBM_SHM_BUFFER_H_

#include "mir/graphics/buffer_basic.h"
#include "mir/geometry/dimensions.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/texture.h"

#include <GLES2/gl2.h>

#include <mutex>

namespace mir
{
class ShmFile;

namespace graphics
{
namespace common
{
class EGLContextExecutor;

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
        std::shared_ptr<EGLContextExecutor> egl_delegate);

    /// \note This must be called with a current GL context
    void upload_to_texture(void const* pixels, geometry::Stride const& stride);
private:
    geometry::Size const size_;
    MirPixelFormat const pixel_format_;
    std::shared_ptr<EGLContextExecutor> const egl_delegate;
    std::mutex tex_id_mutex;
    GLuint tex_id{0};
};

class MemoryBackedShmBuffer :
    public ShmBuffer,
    public renderer::software::RWMappableBuffer
{
public:
    MemoryBackedShmBuffer(
        geometry::Size const& size,
        MirPixelFormat const& pixel_format,
        std::shared_ptr<EGLContextExecutor> egl_delegate);

    auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;
    auto map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>> override;

    auto map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;

    void bind() override;

    auto format() const -> MirPixelFormat override { return ShmBuffer::pixel_format(); }
    auto stride() const -> geometry::Stride override { return stride_; }
    auto size() const -> geometry::Size override { return ShmBuffer::size(); }

    MemoryBackedShmBuffer(MemoryBackedShmBuffer const&) = delete;
    MemoryBackedShmBuffer& operator=(MemoryBackedShmBuffer const&) = delete;
private:
    template<typename T>
    class Mapping;

    template<typename T>
    friend class Mapping;

    geometry::Stride const stride_;
    std::unique_ptr<unsigned char[]> const pixels;
    std::mutex uploaded_mutex;
    bool uploaded{false};
};

class MappableBackedShmBuffer :
    public ShmBuffer,
    public renderer::software::RWMappableBuffer
{
public:
    MappableBackedShmBuffer(
        std::shared_ptr<renderer::software::RWMappableBuffer> data,
        std::shared_ptr<EGLContextExecutor> egl_delegate);

    auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;
    auto map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>> override;
    auto map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;

    void bind() override;

    auto format() const -> MirPixelFormat override;
    auto stride() const -> geometry::Stride override;
    auto size() const -> geometry::Size override;

    MappableBackedShmBuffer(MappableBackedShmBuffer const&) = delete;
    MappableBackedShmBuffer& operator=(MappableBackedShmBuffer const&) = delete;
private:
    std::shared_ptr<renderer::software::RWMappableBuffer> const data;
    std::mutex uploaded_mutex;
    bool uploaded{false};
};

class NotifyingMappableBackedShmBuffer : public MappableBackedShmBuffer
{
public:
    NotifyingMappableBackedShmBuffer(
        std::shared_ptr<renderer::software::RWMappableBuffer> data,
        std::shared_ptr<common::EGLContextExecutor> egl_delegate,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release);

    ~NotifyingMappableBackedShmBuffer() override;

    void bind() override;

    auto map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>> override;
    auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;
    auto map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;

private:
    void notify_consumed();

    std::mutex consumed_mutex;
    std::function<void()> on_consumed;
    std::function<void()> const on_release;
};
}
}
}

#endif /* MIR_GRAPHICS_GBM_SHM_BUFFER_H_ */
