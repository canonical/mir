/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_ZWP_LINUX_DMABUF_V1_H
#define MIR_FRONTEND_ZWP_LINUX_DMABUF_V1_H

#include "wayland.h"
#include "linux_dmabuf_v1.h"

#include <mir/fd.h>
#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/drm_formats.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace mir
{
namespace graphics
{
class Buffer;
class DMABufEGLProvider;
}

namespace frontend
{
/// The per-plane parameters accumulated by a zwp_linux_buffer_params_v1.
using DmaBufPlaneInfo = graphics::DMABufBuffer::PlaneDescriptor;

/**
 * A committed linux-dmabuf buffer.
 *
 * This is both a Wayland buffer (as seen by wl_surface.commit) and a
 * graphics::DMABufBuffer describing the imported dma-buf planes. On commit the
 * surface downcasts the committed buffer to this type and calls import() to turn
 * the dma-buf(s) into a real graphics::Buffer via the DMABufEGLProvider.
 */
class LinuxDmaBufBuffer : public wayland::Buffer, public graphics::DMABufBuffer
{
public:
    LinuxDmaBufBuffer(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::BufferMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<graphics::DMABufEGLProvider> provider,
        geometry::Size size,
        graphics::DRMFormat format,
        uint32_t flags,
        uint64_t modifier,
        std::vector<DmaBufPlaneInfo> planes);

    /// Import the dma-buf(s) into a graphics::Buffer, or nullptr on failure.
    auto import(
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) -> std::shared_ptr<graphics::Buffer>;

    auto size() const -> geometry::Size override;
    auto layout() const -> graphics::gl::Texture::Layout override;
    auto format() const -> graphics::DRMFormat override;
    auto modifier() const -> std::optional<uint64_t> override;
    auto planes() const -> std::vector<DmaBufPlaneInfo> const& override;

private:
    std::shared_ptr<graphics::DMABufEGLProvider> const provider;
    geometry::Size const size_;
    graphics::DRMFormat const format_;
    uint32_t const flags;
    uint64_t const modifier_;
    std::vector<DmaBufPlaneInfo> const planes_;
};

/// Accumulates plane parameters and mints wl_buffers (zwp_linux_buffer_params_v1).
class LinuxBufferParamsV1 : public wayland::LinuxBufferParamsV1
{
public:
    LinuxBufferParamsV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::LinuxBufferParamsV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<graphics::DMABufEGLProvider> provider);

    void add(
        int32_t fd,
        uint32_t plane_idx,
        uint32_t offset,
        uint32_t stride,
        uint32_t modifier_hi,
        uint32_t modifier_lo) override;

    void create(int32_t width, int32_t height, uint32_t format, uint32_t flags) override;

    auto create_immed(
        int32_t width, int32_t height, uint32_t format, uint32_t flags,
        rust::Box<wayland::BufferMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::Buffer> override;

private:
    /* EGL_EXT_image_dma_buf_import allows up to 3 planes, and
     * EGL_EXT_image_dma_buf_import_modifiers adds an extra plane.
     */
    std::array<DmaBufPlaneInfo, 4> planes;
    std::optional<uint64_t> modifier;
    bool consumed;
    std::shared_ptr<graphics::DMABufEGLProvider> const provider;

    void validate_params(int32_t width, int32_t height, uint32_t format, uint32_t flags);
    auto validate_and_collect_planes() -> std::vector<DmaBufPlaneInfo>;
};

/// Sends the format/modifier feedback table (zwp_linux_dmabuf_feedback_v1).
class LinuxDmabufFeedbackV1 : public wayland::LinuxDmabufFeedbackV1
{
public:
    LinuxDmabufFeedbackV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::LinuxDmabufFeedbackV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<graphics::DMABufEGLProvider> provider);
};

/// The zwp_linux_dmabuf_v1 global instance (one per client bind).
class LinuxDmabufV1 : public wayland::LinuxDmabufV1
{
public:
    LinuxDmabufV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::LinuxDmabufV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<graphics::DMABufEGLProvider> provider);

    auto create_params(
        rust::Box<wayland::LinuxBufferParamsV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::LinuxBufferParamsV1> override;

    auto get_default_feedback(
        rust::Box<wayland::LinuxDmabufFeedbackV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::LinuxDmabufFeedbackV1> override;

    auto get_surface_feedback(
        wayland::Weak<wayland::Surface> const& surface,
        rust::Box<wayland::LinuxDmabufFeedbackV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::LinuxDmabufFeedbackV1> override;

private:
    void send_formats();

    std::shared_ptr<graphics::DMABufEGLProvider> const provider;
};
}
}

#endif // MIR_FRONTEND_ZWP_LINUX_DMABUF_V1_H
