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
#include "linux-dmabuf-stable-v1_wrapper.h"

#include <EGL/egl.h>
#include <memory>
#include <span>

#include "mir/graphics/buffer.h"
#include "mir/graphics/drm_formats.h"
#include "mir/graphics/egl_extensions.h"

namespace mir
{
namespace renderer
{
namespace gl
{
class Context;
}
}

namespace graphics
{
namespace gl
{
class Texture;
}

namespace common
{
class EGLContextExecutor;
}

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
        std::function<void()>&& on_release)
        -> std::shared_ptr<Buffer>;

    /**
     * Validate that this provider *can* import this DMA-BUF
     *
     * @param dma_buf
     * \throws  EGL exception on failure
     */
    void validate_import(DMABufBuffer const& dma_buf);

    auto as_texture(
        std::shared_ptr<NativeBufferBase> buffer)
        -> std::shared_ptr<gl::Texture>;

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

class LinuxDmaBuf : public mir::wayland::LinuxDmabufV1::Global
{
public:
    LinuxDmaBuf(
        wl_display* display,
        std::shared_ptr<DMABufEGLProvider> provider);

    auto buffer_from_resource(
        wl_resource* buffer,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release,
        std::shared_ptr<common::EGLContextExecutor> egl_delegate)
        -> std::shared_ptr<Buffer>;
private:
    class Instance;
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<DMABufEGLProvider> const provider;
};

}
}

#endif //MIR_PLATFORM_GBM_KMS_LINUX_DMABUF_H_
