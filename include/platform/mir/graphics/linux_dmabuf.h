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

#ifndef MIR_PLATFORM_GBM_KMS_LINUX_DMABUF_H_
#define MIR_PLATFORM_GBM_KMS_LINUX_DMABUF_H_

#include "egl_context_executor.h"

#include <EGL/egl.h>
#include <sys/types.h>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include <mir/graphics/buffer.h>
#include <mir/graphics/drm_formats.h>
#include <mir/graphics/egl_extensions.h>

namespace mir
{
namespace renderer
{
namespace gl { class Context; }
}

namespace graphics
{
namespace gl { class Texture; }

namespace common { class EGLContextExecutor; }

class DmaBufFormatDescriptors;
class DMABufBuffer;
class EGLBufferCopier;

class DMABufEGLProvider : public std::enable_shared_from_this<DMABufEGLProvider>
{
public:
    using EGLImageAllocator =
        std::function<std::shared_ptr<DMABufBuffer>(DRMFormat, std::span<uint64_t const>, geometry::Size)>;
    DMABufEGLProvider(
        EGLDisplay dpy,
        std::shared_ptr<EGLExtensions> egl_extensions,
        EGLExtensions::EXTImageDmaBufImportModifiers const& dmabuf_ext,
        std::shared_ptr<common::EGLContextExecutor> egl_delegate,
        EGLImageAllocator allocate_importable_image);

    ~DMABufEGLProvider();

    auto devnum() const -> dev_t;

    auto import_dma_buf(
        DMABufBuffer const& dma_buf,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) -> std::shared_ptr<Buffer>;

    /**
     * Validate that this provider *can* import this DMA-BUF
     *
     * @param dma_buf
     * \throws  EGL exception on failure
     */
    void validate_import(DMABufBuffer const& dma_buf);

    auto as_texture(std::shared_ptr<NativeBufferBase> buffer) -> std::shared_ptr<gl::Texture>;

    auto supported_formats() const -> DmaBufFormatDescriptors const&;

private:
    EGLDisplay const dpy;
    std::shared_ptr<EGLExtensions> const egl_extensions;
    std::optional<EGLExtensions::MESADmaBufExport> const dmabuf_export_ext;
    dev_t const devnum_;
    std::unique_ptr<DmaBufFormatDescriptors> const formats;
    std::shared_ptr<common::EGLContextExecutor> const egl_delegate;
    EGLImageAllocator allocate_importable_image;
    std::unique_ptr<EGLBufferCopier> const blitter;
};

/**
 * linux-dmabuf client buffer support offered by a graphics platform.
 *
 * A platform that can import linux-dmabuf client buffers constructs one of these and
 * offers it through GraphicBufferAllocator::linux_dmabuf(). The Wayland frontend only
 * advertises zwp_linux_dmabuf_v1 when the platform offers one, and routes all imports
 * of client dma-bufs through it.
 */
class LinuxDmaBuf
{
public:
    struct FormatModifiers
    {
        DRMFormat format;
        std::vector<uint64_t> modifiers;
    };

    /**
     * \param provider          The provider used to import dma-bufs
     * \param import_context    An EGL context, made current by import() while importing
     */
    LinuxDmaBuf(std::shared_ptr<DMABufEGLProvider> provider, std::unique_ptr<renderer::gl::Context> import_context);

    ~LinuxDmaBuf();

    /// The device clients should allocate their dma-bufs on
    auto main_device() const -> dev_t;

    /// The formats, and the modifiers for each, that clients may submit
    auto formats() const -> std::vector<FormatModifiers> const&;

    /// Whether clients may submit dma-bufs with this format/modifier pair
    auto supports(DRMFormat format, uint64_t modifier) const -> bool;

    /**
     * Validate that this dma-buf *can* be imported
     *
     * \throws  EGL exception on failure
     */
    void validate_import(DMABufBuffer const& dma_buf);

    /**
     * Import a client's dma-buf into a Buffer
     *
     * \note    Must be called on the Wayland thread
     */
    auto import(DMABufBuffer const& dma_buf, std::function<void()>&& on_consumed, std::function<void()>&& on_release)
        -> std::shared_ptr<Buffer>;

private:
    std::shared_ptr<DMABufEGLProvider> const provider;
    std::unique_ptr<renderer::gl::Context> const import_context;
    std::vector<FormatModifiers> const formats_;
};

}
}

#endif //MIR_PLATFORM_GBM_KMS_LINUX_DMABUF_H_
