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
#include <memory>
#include <span>
#include <utility>
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

    /**
     * Whether the given format/modifier combination can be imported by this provider.
     *
     * Exposed for the (Rust-backed) Wayland frontend, which no longer has access to the
     * internal EGL format descriptors.
     */
    auto is_importable(DRMFormat format, uint64_t modifier) const -> bool;

    /**
     * The formats (and their supported modifiers) this provider can import.
     *
     * Exposed for the Wayland frontend so it can advertise linux-dmabuf formats/modifiers
     * without touching the internal EGL descriptor types.
     */
    auto supported_format_modifiers() const -> std::vector<std::pair<DRMFormat, std::vector<uint64_t>>>;

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

}
}

#endif //MIR_PLATFORM_GBM_KMS_LINUX_DMABUF_H_
