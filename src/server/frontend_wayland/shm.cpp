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

#include "shm.h"
#include <mir/graphics/drm_formats.h>
#include "../shm_backing.h"
#include <mir/log.h>

#include <mir/executor.h>
#include <mir/renderer/sw/pixel_source.h>

#include <drm_fourcc.h>
#include <boost/throw_exception.hpp>

#include "protocol_error.h"
#include "wayland_rs/src/ffi.rs.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;

mf::ShmBuffer::ShmBuffer(
    rust::Box<wayland_rs::WaylandClient> client,
    std::shared_ptr<shm::RWMappableRange> data,
    geometry::Size size,
    geometry::Stride stride,
    graphics::DRMFormat format)
    : WlBufferImpl(std::move(client)),
      data_{std::move(data)},
      size_{std::move(size)},
      stride_{stride},
      format_{format}
{
}

namespace
{
class ErrorNotifyingRWMappableBuffer : public mrs::RWMappable
{
public:
    ErrorNotifyingRWMappableBuffer(
        std::shared_ptr<mf::ShmBuffer> const& buffer,
        std::shared_ptr<mir::shm::RWMappableRange> data,
        mir::geometry::Size size,
        mir::geometry::Stride stride,
        MirPixelFormat format);

    auto size() const -> mir::geometry::Size override;
    auto stride() const -> mir::geometry::Stride override;
    auto format() const -> MirPixelFormat override;

    auto map_writeable() -> std::unique_ptr<mrs::Mapping<std::byte>> override;
    auto map_rw() -> std::unique_ptr<mrs::Mapping<std::byte>> override;

    void notify_access_error() const;
private:
    std::weak_ptr<mf::ShmBuffer> buffer;
    std::shared_ptr<mir::shm::RWMappableRange> const data;
    mir::geometry::Size const size_;
    mir::geometry::Stride const stride_;
    MirPixelFormat const format_;
};

template<typename T>
class ErrorNotifyingMapping : public mir::renderer::software::Mapping<T>
{
public:
    ErrorNotifyingMapping(
        std::unique_ptr<mir::shm::Mapping<std::conditional_t<std::is_const_v<T>, std::byte const, std::byte>>> mapping,
        ErrorNotifyingRWMappableBuffer const& parent)
        : mapping{std::move(mapping)},
          parent{parent}
    {
    }

    ~ErrorNotifyingMapping()
    {
        if (mapping->access_fault())
        {
            parent.notify_access_error();
        }
    }

    auto format() const -> MirPixelFormat override
    {
        return parent.format();
    }

    auto stride() const -> mir::geometry::Stride override
    {
        return parent.stride();
    }

    auto size() const -> mir::geometry::Size override
    {
        return parent.size();
    }

    auto data() const -> T* override
    {
        return reinterpret_cast<T*>(mapping->data());
    }

    auto len() const -> size_t override
    {
        return mapping->len();
    }

private:
    std::unique_ptr<mir::shm::Mapping<std::conditional_t<std::is_const_v<T>, std::byte const, std::byte>>> const mapping;
    ErrorNotifyingRWMappableBuffer const& parent;
};

ErrorNotifyingRWMappableBuffer::ErrorNotifyingRWMappableBuffer(
    std::shared_ptr<mf::ShmBuffer> const& buffer,
    std::shared_ptr<mir::shm::RWMappableRange> data,
    mir::geometry::Size size,
    mir::geometry::Stride stride,
    MirPixelFormat format)
    : buffer{buffer},
      data{std::move(data)},
      size_{std::move(size)},
      stride_{stride},
      format_{format}
{
}

void ErrorNotifyingRWMappableBuffer::notify_access_error() const
{
    if (auto const locked = buffer.lock())
    {
        locked->post_error(
            mir::wayland_rs::WlShmImpl::Error::invalid_fd,
            "Error accessing SHM buffer");
    }
    else
    {
        mir::log(
            mir::logging::Severity::warning,
            "Client SHM Buffer",
            "Client submitted invalid SHM buffer; rendering will be incomplete");
    }
}

auto ErrorNotifyingRWMappableBuffer::size() const -> mir::geometry::Size
{
    return size_;
}

auto ErrorNotifyingRWMappableBuffer::stride() const -> mir::geometry::Stride
{
    return stride_;
}

auto ErrorNotifyingRWMappableBuffer::format() const -> MirPixelFormat
{
    return format_;
}

auto ErrorNotifyingRWMappableBuffer::map_rw() -> std::unique_ptr<mrs::Mapping<std::byte>>
{
    return std::make_unique<ErrorNotifyingMapping<std::byte>>(data->map_rw(), *this);
}

auto ErrorNotifyingRWMappableBuffer::map_writeable() -> std::unique_ptr<mrs::Mapping<std::byte>>
{
    return std::make_unique<ErrorNotifyingMapping<std::byte>>(data->map_wo(), *this);
}
}

auto mf::ShmBuffer::data() -> std::shared_ptr<mrs::RWMappable>
{
    return std::make_shared<ErrorNotifyingRWMappableBuffer>(
        shared_from_this(),
        data_,
        size_,
        stride_,
        format_.as_mir_format().value());
}

mf::ShmPool::ShmPool(
    rust::Box<wayland_rs::WaylandClient> client,
    mir::Fd backing_store,
    int32_t claimed_size) :
    WlShmPoolImpl(std::move(client)),
    backing_store{shm::rw_pool_from_fd(std::move(backing_store), claimed_size)}
{
}

namespace
{
auto wl_shm_format_to_drm_format(uint32_t format) -> mg::DRMFormat
{
    switch (format)
    {
    /* The Wayland SHM formats are all the same as DRM formats from drm_fourcc.h, with the exception
     * of the two default formats, which are special-cased here.
     */
    case mf::Shm::Format::argb8888:
        return mg::DRMFormat{DRM_FORMAT_ARGB8888};
    case mf::Shm::Format::xrgb8888:
        return mg::DRMFormat{DRM_FORMAT_XRGB8888};
    default:
        return mg::DRMFormat{format};
    }
}
}

auto mf::ShmPool::create_buffer(int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format) -> std::shared_ptr<wayland_rs::WlBufferImpl>
{
    auto const size = stride * height;
    std::unique_ptr<shm::RWMappableRange> backing_range;
    try
    {
        backing_range = backing_store->get_rw_range(offset, size);
    }
    catch (std::logic_error const&)
    {
        /* get_*_range throws a logic error when attempting to access outside the backing
         * store. This should be translated into a ProtocolError.
         */
        throw wayland_rs::ProtocolError{
            object_id(),
            wayland_rs::WlShmImpl::Error::invalid_stride,
            "Attempt to create_buffer outside the range of the backing store"};
    }

    // TODO: Extend DRMFormat to include bytes-per-pixel info and drop this hardcoded "4"
    if (stride < (width * 4))
    {
        throw wayland_rs::ProtocolError{
            object_id(),
            wayland_rs::WlShmImpl::Error::invalid_stride,
            "Invalid stride %d (too small for width %d. Did you specify stride in pixels?)",
            stride, width};
    }

    // TODO: Pull supported formats out of RenderingPlatform to support more than the required formats
    if (format != wayland_rs::WlShmImpl::Format::argb8888 && format != wayland_rs::WlShmImpl::Format::xrgb8888)
    {
        throw wayland_rs::ProtocolError{
            object_id(),
            wayland_rs::WlShmImpl::Error::invalid_format,
            "Invalid SHM format requested"};
    }

    return std::make_shared<ShmBuffer>(
        client->clone_box(),
        std::move(backing_range),
        geometry::Size{width, height},
        geometry::Stride{stride},
        wl_shm_format_to_drm_format(format)
    );
}

void mf::ShmPool::resize(int32_t new_size)
{
    backing_store->resize(new_size);
}

mf::Shm::Shm(rust::Box<wayland_rs::WaylandClient> client)
    : WlShmImpl(std::move(client))
{
    // TODO: send all the formats we support, beyond the mandatory ones.
    for (auto format : { Format::argb8888, Format::xrgb8888 })
    {
        send_format_event(format);
    }
}

auto mf::Shm::create_pool(int32_t fd, int32_t size) -> std::shared_ptr<wayland_rs::WlShmPoolImpl>
{
    if (size <= 0)
    {
        throw wayland_rs::ProtocolError{
            object_id(),
            Error::invalid_stride,
            "Invalid requested size"};
    }

    return std::make_shared<ShmPool>(client->clone_box(), Fd{fd}, size);
}
