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
#include <map>

namespace mir
{
class ShmFile;

namespace graphics
{
class RenderingProvider;

namespace common
{
class EGLContextExecutor;

class ShmBuffer :
    public BufferBasic,
    public NativeBufferBase
{
public:
    ~ShmBuffer() noexcept override;

    static bool supports(MirPixelFormat);

    geometry::Size size() const override;
    MirPixelFormat pixel_format() const override;
    NativeBufferBase* native_buffer_base() override;

    auto texture_for_provider(
        std::shared_ptr<EGLContextExecutor> const& egl_delegate,
        RenderingProvider* provider) -> std::shared_ptr<gl::Texture>;

protected:
    ShmBuffer(
        geometry::Size const& size,
        MirPixelFormat const& format);

    class ShmBufferTexture : public gl::Texture
    {
    public:
        explicit ShmBufferTexture(std::shared_ptr<EGLContextExecutor> const& egl_delegate);
        ~ShmBufferTexture() override;
        void bind() override;
        auto tex_id() const -> GLuint override;
        gl::Program const& shader(gl::ProgramFactory& factory) const override;
        Layout layout() const override;
        void add_syncpoint() override;
        void try_upload_to_texture(
            BufferID id,
            void const* pixels,
            geometry::Size const& size,
            geometry::Stride const& stride,
            MirPixelFormat pixel_format);
        void mark_dirty();

    private:
        std::shared_ptr<EGLContextExecutor> egl_delegate;
        GLuint tex_id_;
        std::mutex uploaded_mutex;
        bool uploaded = false;
    };

    virtual void on_texture_accessed(std::shared_ptr<ShmBufferTexture> const&) = 0;

    std::mutex tex_mutex;
    std::map<RenderingProvider*, std::shared_ptr<ShmBufferTexture>> provider_to_texture_map;
private:
    geometry::Size const size_;
    MirPixelFormat const pixel_format_;
};

class MemoryBackedShmBuffer :
    public ShmBuffer,
    public renderer::software::RWMappableBuffer
{
public:
    MemoryBackedShmBuffer(
        geometry::Size const& size,
        MirPixelFormat const& pixel_format);

    auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;
    auto map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>> override;
    auto map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;

    auto format() const -> MirPixelFormat override { return ShmBuffer::pixel_format(); }
    auto stride() const -> geometry::Stride override { return stride_; }
    auto size() const -> geometry::Size override { return ShmBuffer::size(); }
    void mark_dirty();

    MemoryBackedShmBuffer(MemoryBackedShmBuffer const&) = delete;
    MemoryBackedShmBuffer& operator=(MemoryBackedShmBuffer const&) = delete;

protected:
    void on_texture_accessed(std::shared_ptr<ShmBufferTexture> const&) override;

private:
    template<typename T>
    class Mapping;

    template<typename T>
    friend class Mapping;

    geometry::Stride const stride_;
    std::unique_ptr<unsigned char[]> const pixels;
};

class MappableBackedShmBuffer :
    public ShmBuffer,
    public renderer::software::RWMappableBuffer
{
public:
    MappableBackedShmBuffer(
        std::shared_ptr<RWMappableBuffer> data);

    auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;
    auto map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>> override;
    auto map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;

    auto format() const -> MirPixelFormat override;
    auto stride() const -> geometry::Stride override;
    auto size() const -> geometry::Size override;

    MappableBackedShmBuffer(MappableBackedShmBuffer const&) = delete;
    MappableBackedShmBuffer& operator=(MappableBackedShmBuffer const&) = delete;

protected:
    void on_texture_accessed(std::shared_ptr<ShmBufferTexture> const&) override;

private:
    std::shared_ptr<RWMappableBuffer> const data;
};

class NotifyingMappableBackedShmBuffer : public MappableBackedShmBuffer
{
public:
    NotifyingMappableBackedShmBuffer(
        std::shared_ptr<renderer::software::RWMappableBuffer> data,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release);

    ~NotifyingMappableBackedShmBuffer() override;

    auto map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>> override;
    auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;
    auto map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;

protected:
    void on_texture_accessed(std::shared_ptr<ShmBufferTexture> const&) override;

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
