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

#include "zwp_linux_dmabuf_v1.h"

#include "wayland.h"
#include "client.h"
#include "protocol_error.h"
#include "wayland_rs/src/ffi.rs.h"

#include <mir/anonymous_shm_file.h>
#include <mir/fd.h>
#include <mir/graphics/buffer.h>
#include <mir/graphics/egl_error.h>
#include <mir/graphics/linux_dmabuf.h>
#include <mir/log.h>
#include <mir/raii.h>
#include <mir/renderer/gl/context.h>

#include <algorithm>
#include <cstring>
#include <system_error>
#include <unistd.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mwrs = mir::wayland_rs;
namespace geom = mir::geometry;

mf::LinuxDmaBufBuffer::LinuxDmaBufBuffer(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::BufferMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<mg::DMABufEGLProvider> provider,
    geom::Size size,
    mg::DRMFormat format,
    uint32_t flags,
    uint64_t modifier,
    std::vector<DmaBufPlaneInfo> planes)
    : wayland_rs::Buffer(std::move(client), std::move(instance), object_id),
      provider{std::move(provider)},
      size_{size},
      format_{format},
      flags{flags},
      modifier_{modifier},
      planes_{std::move(planes)}
{
}

auto mf::LinuxDmaBufBuffer::import(
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release) -> std::shared_ptr<mg::Buffer>
{
    /* import_dma_buf() creates a GL texture bound to the imported EGLImage, which
     * requires a current EGL context. The Wayland commit thread has none, so make the
     * provider's import context current for the duration of the import.
     */
    auto& ctx = provider->context();
    auto const context_guard = mir::raii::paired_calls(
        [&ctx]() { ctx.make_current(); },
        [&ctx]() { ctx.release_current(); });
    return provider->import_dma_buf(*this, std::move(on_consumed), std::move(on_release));
}

auto mf::LinuxDmaBufBuffer::size() const -> geom::Size
{
    return size_;
}

auto mf::LinuxDmaBufBuffer::layout() const -> mg::gl::Texture::Layout
{
    if (flags & mwrs::LinuxBufferParamsV1::Flags::y_invert)
        return mg::gl::Texture::Layout::TopRowFirst;
    return mg::gl::Texture::Layout::GL;
}

auto mf::LinuxDmaBufBuffer::format() const -> mg::DRMFormat
{
    return format_;
}

auto mf::LinuxDmaBufBuffer::modifier() const -> std::optional<uint64_t>
{
    return modifier_;
}

auto mf::LinuxDmaBufBuffer::planes() const -> std::vector<DmaBufPlaneInfo> const&
{
    return planes_;
}

mf::LinuxBufferParamsV1::LinuxBufferParamsV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::LinuxBufferParamsV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<mg::DMABufEGLProvider> provider)
    : wayland_rs::LinuxBufferParamsV1(std::move(client), std::move(instance), object_id),
      planes{},
      consumed{false},
      provider{std::move(provider)}
{
}

void mf::LinuxBufferParamsV1::add(
    int32_t fd,
    uint32_t plane_idx,
    uint32_t offset,
    uint32_t stride,
    uint32_t modifier_hi,
    uint32_t modifier_lo)
{
    // The fd is borrowed for the duration of this call; take our own copy.
    mir::Fd owned_fd{::dup(fd)};

    if (consumed)
        throw mwrs::ProtocolError{
            object_id(), Error::already_used, "Params already used to create a buffer"};

    if (plane_idx >= planes.size())
        throw mwrs::ProtocolError{
            object_id(), Error::plane_idx,
            "Plane index %u higher than maximum number of planes, %zu",
            plane_idx, planes.size()};

    if (planes[plane_idx].dma_buf != mir::Fd::invalid)
        throw mwrs::ProtocolError{
            object_id(), Error::plane_set, "Plane %u already has a dmabuf", plane_idx};

    planes[plane_idx].dma_buf = std::move(owned_fd);
    planes[plane_idx].offset = offset;
    planes[plane_idx].stride = stride;

    auto const new_modifier = (static_cast<uint64_t>(modifier_hi) << 32) | modifier_lo;
    if (modifier)
    {
        if (*modifier != new_modifier)
            throw mwrs::ProtocolError{
                object_id(), Error::invalid_format,
                "Modifier for plane %u doesn't match previously set modifier -"
                " all planes must use the same modifier", plane_idx};
    }
    else
    {
        modifier = new_modifier;
    }
}

auto mf::LinuxBufferParamsV1::validate_and_collect_planes() -> std::vector<mf::DmaBufPlaneInfo>
{
    if (consumed)
        throw mwrs::ProtocolError{
            object_id(), Error::already_used, "Params already used to create a buffer"};

    auto const plane_count = std::count_if(
        planes.begin(), planes.end(),
        [](auto const& plane) { return plane.dma_buf != mir::Fd::invalid; });

    if (plane_count == 0)
        throw mwrs::ProtocolError{
            object_id(), Error::incomplete, "No dmabuf has been added to the params"};

    for (auto i = 0; i != plane_count; ++i)
    {
        if (planes[i].dma_buf == mir::Fd::invalid)
            throw mwrs::ProtocolError{
                object_id(), Error::incomplete, "Missing dmabuf for plane %u", i};
    }

    return {planes.cbegin(), planes.cbegin() + plane_count};
}

void mf::LinuxBufferParamsV1::validate_params(
    int32_t width, int32_t height, uint32_t format, uint32_t /*flags*/)
{
    if (width < 1 || height < 1)
        throw mwrs::ProtocolError{
            object_id(), Error::invalid_dimensions,
            "Width %i or height %i invalid; both must be >= 1!", width, height};

    if (!provider || !provider->is_importable(mg::DRMFormat{format}, modifier.value()))
        throw mwrs::ProtocolError{
            object_id(), Error::invalid_format,
            "Client requested unsupported format/modifier combination %s/%s",
            mg::DRMFormat{format}.name(),
            mg::drm_modifier_to_string(modifier.value()).c_str()};
}

void mf::LinuxBufferParamsV1::create(
    int32_t width, int32_t height, uint32_t format, uint32_t flags)
{
    auto plane_infos = validate_and_collect_planes();
    validate_params(width, height, format, flags);
    consumed = true;

    mg::DRMFormat const drm_format{format};
    std::shared_ptr<LinuxDmaBufBuffer> created;
    // Ownership of the newly-created wl_buffer passes to the Rust server.
    mwrs::create_wl_buffer(
        *client->raw_client(), 1,
        [&](rust::Box<mwrs::BufferMiddleware> box, uint32_t buffer_object_id)
            -> std::shared_ptr<mwrs::Buffer>
        {
            created = std::make_shared<LinuxDmaBufBuffer>(
                client, std::move(box), buffer_object_id, provider,
                geom::Size{width, height}, drm_format, flags, modifier.value(), plane_infos);
            return created;
        });

    try
    {
        // We don't need to keep it around, but we do need to ensure that we *can*
        // create a Buffer from this dma-buf.
        provider->validate_import(*created);
        send_created_event(created->get_box());
    }
    catch (std::system_error const& err)
    {
        if (err.code().category() != mg::egl_category())
            throw;
        mir::log_debug("Failed to import client dmabufs: %s", err.what());
        send_failed_event();
    }
}

auto mf::LinuxBufferParamsV1::create_immed(
    int32_t width, int32_t height, uint32_t format, uint32_t flags,
    rust::Box<mwrs::BufferMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::Buffer>
{
    auto plane_infos = validate_and_collect_planes();
    validate_params(width, height, format, flags);
    consumed = true;

    mg::DRMFormat const drm_format{format};
    auto buffer = std::make_shared<LinuxDmaBufBuffer>(
        client, std::move(child_instance), child_object_id, provider,
        geom::Size{width, height}, drm_format, flags, modifier.value(), std::move(plane_infos));

    try
    {
        // We don't need to keep it around, but we do need to ensure that we *can*
        // create a Buffer from this dma-buf.
        provider->validate_import(*buffer);
    }
    catch (std::system_error const& err)
    {
        if (err.code().category() != mg::egl_category())
            throw;
        /* The protocol gives implementations the choice of sending an invalid_wl_buffer
         * protocol error and disconnecting the client here, or sending a failed() event
         * and not disconnecting the client. Both Weston and GNOME Shell choose to
         * disconnect; follow their lead.
         */
        throw mwrs::ProtocolError{
            object_id(), Error::invalid_wl_buffer, "Failed to import dmabuf: %s", err.what()};
    }

    return buffer;
}

mf::LinuxDmabufFeedbackV1::LinuxDmabufFeedbackV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::LinuxDmabufFeedbackV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<mg::DMABufEGLProvider> provider)
    : wayland_rs::LinuxDmabufFeedbackV1(std::move(client), std::move(instance), object_id)
{
    if (!provider)
    {
        send_done_event();
        return;
    }

    struct FormatTableEntry
    {
        uint32_t format;
        uint32_t padding; /* unused */
        uint64_t modifier;
    };

    auto const format_modifiers = provider->supported_format_modifiers();
    size_t format_table_length = 0;
    for (auto const& [format, modifiers] : format_modifiers)
        format_table_length += modifiers.size();

    mir::AnonymousShmFile shm_buffer{format_table_length * sizeof(FormatTableEntry)};
    auto* const data = static_cast<FormatTableEntry*>(shm_buffer.base_ptr());
    size_t k = 0;
    for (auto const& [format, modifiers] : format_modifiers)
    {
        for (auto const modifier : modifiers)
        {
            data[k].format = static_cast<uint32_t>(format);
            data[k].padding = 0;
            data[k].modifier = modifier;
            ++k;
        }
    }

    // The fd is only borrowed for the duration of the send; the AnonymousShmFile
    // stays alive for the whole constructor.
    send_format_table_event(shm_buffer.fd(), format_table_length * sizeof(FormatTableEntry));

    auto const devnum = provider->devnum();
    std::vector<uint8_t> device_bytes(sizeof(devnum));
    std::memcpy(device_bytes.data(), &devnum, sizeof(devnum));
    send_main_device_event(device_bytes);

    // We only currently support one device, which accesses all formats.
    send_tranche_target_device_event(device_bytes);
    send_tranche_flags_event(0);

    std::vector<uint8_t> indices;
    indices.reserve(format_table_length * sizeof(uint16_t));
    for (uint16_t i = 0; i < static_cast<uint16_t>(format_table_length); ++i)
    {
        auto const* const bytes = reinterpret_cast<uint8_t const*>(&i);
        indices.insert(indices.end(), bytes, bytes + sizeof(uint16_t));
    }
    send_tranche_formats_event(indices);
    send_tranche_done_event();

    send_done_event();
}

mf::LinuxDmabufV1::LinuxDmabufV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::LinuxDmabufV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<mg::DMABufEGLProvider> provider)
    : wayland_rs::LinuxDmabufV1(std::move(client), std::move(instance), object_id),
      provider{std::move(provider)}
{
    if (get_box()->version() < 4)
        send_formats();
}

void mf::LinuxDmabufV1::send_formats()
{
    if (!provider)
        return;

    for (auto const& [format, modifiers] : provider->supported_format_modifiers())
    {
        send_format_event(static_cast<uint32_t>(format));
        for (auto const modifier : modifiers)
        {
            send_modifier_event_if_supported(
                static_cast<uint32_t>(format),
                static_cast<uint32_t>(modifier >> 32),
                static_cast<uint32_t>(modifier & 0xFFFFFFFF));
        }
    }
}

auto mf::LinuxDmabufV1::create_params(
    rust::Box<mwrs::LinuxBufferParamsV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::LinuxBufferParamsV1>
{
    return std::make_shared<LinuxBufferParamsV1>(
        client, std::move(child_instance), child_object_id, provider);
}

auto mf::LinuxDmabufV1::get_default_feedback(
    rust::Box<mwrs::LinuxDmabufFeedbackV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::LinuxDmabufFeedbackV1>
{
    return std::make_shared<LinuxDmabufFeedbackV1>(
        client, std::move(child_instance), child_object_id, provider);
}

auto mf::LinuxDmabufV1::get_surface_feedback(
    mwrs::Weak<mwrs::Surface> const& /*surface*/,
    rust::Box<mwrs::LinuxDmabufFeedbackV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::LinuxDmabufFeedbackV1>
{
    // We have the same feedback regardless of surface.
    return std::make_shared<LinuxDmabufFeedbackV1>(
        client, std::move(child_instance), child_object_id, provider);
}
