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
#include "../shm_backing.h"

#include "protocol_error.h"

#include <mir/fd.h>
#include <mir/log.h>
#include <mir/executor.h>
#include <mir/renderer/sw/pixel_source.h>

#include <drm_fourcc.h>
#include <boost/throw_exception.hpp>
#include <unistd.h>

#include <algorithm>
#include <expected>
#include <limits>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace mwrs = mir::wayland_rs;

mf::ShmBuffer::ShmBuffer(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::BufferMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<mir::Executor> wayland_executor,
    std::shared_ptr<shm::RWMappableRange> data,
    geometry::Size size,
    geometry::Stride stride,
    graphics::DRMFormat format)
    : Buffer{std::move(client), std::move(instance), object_id},
      weak_me{mwrs::make_weak(this)},
      wayland_executor{std::move(wayland_executor)},
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
        mwrs::Weak<mf::ShmBuffer> buffer,
        std::shared_ptr<mir::Executor> wayland_executor,
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
    mwrs::Weak<mf::ShmBuffer> const weak_buffer;
    std::shared_ptr<mir::Executor> const wayland_executor;
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
    mwrs::Weak<mf::ShmBuffer> buffer,
    std::shared_ptr<mir::Executor> wayland_executor,
    std::shared_ptr<mir::shm::RWMappableRange> data,
    mir::geometry::Size size,
    mir::geometry::Stride stride,
    MirPixelFormat format)
    : weak_buffer{buffer},
      wayland_executor{std::move(wayland_executor)},
      data{std::move(data)},
      size_{std::move(size)},
      stride_{stride},
      format_{format}
{
}

void ErrorNotifyingRWMappableBuffer::notify_access_error() const
{
    wayland_executor->spawn(
        [buffer = weak_buffer]()
        {
            if (buffer)
            {
                buffer.value().post_error(
                    mwrs::Shm::Error::invalid_fd,
                    "Error accessing SHM buffer");
            }
            else
            {
                mir::log(
                    mir::logging::Severity::warning,
                    "Client SHM Buffer",
                    "Client submitted invalid SHM buffer; rendering will be incomplete");
            }
        });
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
        weak_me,
        wayland_executor,
        data_,
        size_,
        stride_,
        format_.as_mir_format().value());
}

mf::ShmPool::ShmPool(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ShmPoolMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<std::vector<mg::DRMFormat> const> supported_formats,
    Fd backing_store,
    int32_t claimed_size) :
    wayland_rs::ShmPool{std::move(client), std::move(instance), object_id},
    wayland_executor{std::move(wayland_executor)},
    supported_formats{std::move(supported_formats)},
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

auto drm_format_to_wl_shm_format(mg::DRMFormat format) -> uint32_t
{
    /* Inverse of wl_shm_format_to_drm_format(): the two default formats use the
     * special-cased wl_shm codes, everything else matches its DRM fourcc.
     */
    switch (static_cast<uint32_t>(format))
    {
    case DRM_FORMAT_ARGB8888:
        return mf::Shm::Format::argb8888;
    case DRM_FORMAT_XRGB8888:
        return mf::Shm::Format::xrgb8888;
    default:
        return static_cast<uint32_t>(format);
    }
}
}

auto mf::ShmPool::create_buffer(
    int32_t offset,
    int32_t width, int32_t height,
    int32_t stride,
    uint32_t format,
    rust::Box<mwrs::BufferMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::Buffer>
{
    if (offset < 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_stride,
            "Invalid SHM buffer offset %d", offset};
    }
    if (width <= 0 || height <= 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_stride,
            "Invalid SHM buffer size %dx%d", width, height};
    }
    if (stride <= 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_stride,
            "Invalid SHM buffer stride %d", stride};
    }

    auto const drm_format = wl_shm_format_to_drm_format(format);
    auto const format_supported = std::any_of(
        supported_formats->begin(),
        supported_formats->end(),
        [&drm_format](auto supported) { return supported == drm_format; });
    if (!format_supported)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_format,
            "Invalid SHM format %u", format};
    }

    auto const format_info = drm_format.info();
    if (!format_info)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_format,
            "Missing pixel-size information for SHM format requested"};
    }
    // Casting to stop the below calculations being unsigned and then comparing against signed width/height.
    auto const bytes_per_pixel = static_cast<geometry::Size::ValueType>(format_info->bytes_per_pixel());

    auto const max_size = std::numeric_limits<geometry::Size::ValueType>::max();
    auto const max_width = max_size / bytes_per_pixel;
    auto const max_height = max_size / stride;
    if (width > max_width || height > max_height)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_stride,
            "Requested SHM buffer size %dx%d (stride %d) is too large", width, height, stride};
    }

    auto const min_stride = width * bytes_per_pixel;
    if (stride < min_stride)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_stride,
            "Invalid stride %d (too small for width %d. Did you specify stride in pixels?)",
            stride, width};
    }

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
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_stride,
            "Attempt to create_buffer outside the range of the backing store"};
    }

    return std::make_shared<ShmBuffer>(
        client,
        std::move(child_instance),
        child_object_id,
        wayland_executor,
        std::move(backing_range),
        geometry::Size{width, height},
        geometry::Stride{stride},
        drm_format);
}

void mf::ShmPool::resize(int32_t new_size)
{
    if (new_size < 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_stride,
            "Invalid new size %d", new_size};
    }

    if (auto result = backing_store->resize(new_size); !result)
    {
        switch(result.error())
        {
        case shm::ResizeError::invalid_size:
            throw mwrs::ProtocolError{
                object_id(),
                mwrs::Shm::Error::invalid_stride,
                "New size %d is smaller than the current size of the backing store", new_size};
            break;
        }
    }
}

mf::Shm::Shm(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::ShmMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<std::vector<mg::DRMFormat> const> supported_formats)
    : wayland_rs::Shm{std::move(client), std::move(instance), object_id},
      wayland_executor{std::move(wayland_executor)},
      supported_formats{std::move(supported_formats)}
{
    for (auto const& format : *this->supported_formats)
    {
        send_format_event(drm_format_to_wl_shm_format(format));
    }
}

auto mf::Shm::create_pool(
    int32_t fd,
    int32_t size,
    rust::Box<mwrs::ShmPoolMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::ShmPool>
{
    if (size <= 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_stride,
            "Invalid requested size"};
    }

    // The fd is borrowed from the Rust dispatch layer and is only valid for the duration of this
    // call; dup() it so the pool can retain ownership of the backing store. The dup()ed fd is wrapped
    // in a mir::Fd via the owning `int` constructor so that it is close()d when the pool (and hence
    // the backing store) is destroyed. mir::IntOwnedFd must *not* be used here: it produces a
    // non-owning mir::Fd that never close()s the descriptor, leaking one fd per pool.
    mir::Fd owned_fd{::dup(fd)};
    if (owned_fd == mir::Fd::invalid)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_fd,
            "Failed to dup() client-provided SHM pool fd"};
    }

    try
    {
        return std::make_shared<ShmPool>(
            client,
            std::move(child_instance),
            child_object_id,
            wayland_executor,
            supported_formats,
            std::move(owned_fd),
            size);
    }
    catch (shm::MmapError const& err)
    {
        // The pool is backed by mmap()ing the client-provided file descriptor. Per the wl_shm spec a
        // failure to mmap the file descriptor must be reported with the invalid_fd error.
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::Shm::Error::invalid_fd,
            "%s",
            err.what()};
    }
}
