#include "shm.h"
#include "mir/graphics/drm_formats.h"
#include "mir/log.h"
#include "shm_backing.h"

#include "mir/wayland/weak.h"
#include "mir/executor.h"
#include "mir/renderer/sw/pixel_source.h"

#include <drm/drm_fourcc.h>
#include <boost/throw_exception.hpp>
#include <wayland-server-protocol.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;

mf::ShmBuffer::ShmBuffer(
    struct wl_resource* resource,
    std::shared_ptr<mir::Executor> wayland_executor,
    std::shared_ptr<RWMappableRange> data,
    geometry::Size size,
    geometry::Stride stride,
    graphics::DRMFormat format)
    : Buffer{resource, Version<1>{}},
      weak_me{wayland::make_weak(this)},
      wayland_executor{std::move(wayland_executor)},
      data_{std::move(data)},
      size_{std::move(size)},
      stride_{stride},
      format_{format}
{
}

namespace
{
class ErrorNotifyingRWMappableBuffer : public mrs::RWMappableBuffer
{
public:
    ErrorNotifyingRWMappableBuffer(
        mir::wayland::Weak<mf::ShmBuffer> buffer,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::shared_ptr<mir::RWMappableRange> data,
        mir::geometry::Size size,
        mir::geometry::Stride stride,
        MirPixelFormat format);

    auto size() const -> mir::geometry::Size override;
    auto stride() const -> mir::geometry::Stride override;
    auto format() const -> MirPixelFormat override;

    auto map_readable() -> std::unique_ptr<mrs::Mapping<unsigned char const>> override;
    auto map_writeable() -> std::unique_ptr<mrs::Mapping<unsigned char>> override;
    auto map_rw() -> std::unique_ptr<mrs::Mapping<unsigned char>> override;

    void notify_access_error() const;
private:
    mir::wayland::Weak<mf::ShmBuffer> const weak_buffer;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<mir::RWMappableRange> const data;
    mir::geometry::Size const size_;
    mir::geometry::Stride const stride_;
    MirPixelFormat const format_;
};

template<typename T>
class ErrorNotifyingMapping : public mir::renderer::software::Mapping<T>
{
public:
    ErrorNotifyingMapping(
        std::unique_ptr<mir::Mapping<std::conditional_t<std::is_const_v<T>, std::byte const, std::byte>>> mapping,
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

    auto data() -> T* override
    {
        return reinterpret_cast<T*>(mapping->data());
    }

    auto len() const -> size_t override
    {
        return mapping->len();
    }

private:
    std::unique_ptr<mir::Mapping<std::conditional_t<std::is_const_v<T>, std::byte const, std::byte>>> const mapping;
    ErrorNotifyingRWMappableBuffer const& parent;
};

ErrorNotifyingRWMappableBuffer::ErrorNotifyingRWMappableBuffer(
        mir::wayland::Weak<mf::ShmBuffer> buffer,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::shared_ptr<mir::RWMappableRange> data,
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
                wl_resource_post_error(
                    buffer.value().resource,
                    mir::wayland::Shm::Error::invalid_fd,
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

auto ErrorNotifyingRWMappableBuffer::map_rw() -> std::unique_ptr<mrs::Mapping<unsigned char>>
{
    return std::make_unique<ErrorNotifyingMapping<unsigned char>>(data->map_rw(), *this);
}

auto ErrorNotifyingRWMappableBuffer::map_readable() -> std::unique_ptr<mrs::Mapping<unsigned char const>>
{
    return std::make_unique<ErrorNotifyingMapping<unsigned char const>>(data->map_ro(), *this);
}

auto ErrorNotifyingRWMappableBuffer::map_writeable() -> std::unique_ptr<mrs::Mapping<unsigned char>>
{
    return std::make_unique<ErrorNotifyingMapping<unsigned char>>(data->map_wo(), *this);
}
}

auto mf::ShmBuffer::data() -> std::shared_ptr<mrs::RWMappableBuffer>
{
    return std::make_shared<ErrorNotifyingRWMappableBuffer>(
        wayland::make_weak<mf::ShmBuffer>(this),
        wayland_executor,
        data_,
        size_,
        stride_,
        format_.as_mir_format().value());
}

auto mf::ShmBuffer::from(wl_resource* resource) -> ShmBuffer*
{
    if (auto buffer = wayland::Buffer::from(resource))
    {
        return dynamic_cast<ShmBuffer*>(buffer);
    }
    return nullptr;
}

mf::ShmPool::ShmPool(
    struct wl_resource* resource,
    std::shared_ptr<Executor> wayland_executor,
    Fd backing_store,
    int32_t claimed_size) :
    wayland::ShmPool(resource, Version<1>{}),
    wayland_executor{std::move(wayland_executor)},
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

void mf::ShmPool::create_buffer(
    struct wl_resource* id,
    int32_t offset,
    int32_t width, int32_t height,
    int32_t stride,
    uint32_t format)
{
    auto const size = stride * height;
    auto backing_range = backing_store->get_rw_range(offset, size);
    new ShmBuffer{
        id,
        wayland_executor,
        std::move(backing_range),
        geometry::Size{width, height},
        geometry::Stride{stride},
        wl_shm_format_to_drm_format(format)
    };
}

void mf::ShmPool::resize(int32_t new_size)
{
    backing_store->resize(new_size);
}

mf::WlShm::WlShm(wl_display* display, std::shared_ptr<Executor> wayland_executor)
    : wayland::Shm::Global(display, Version<1>{}),
      wayland_executor{std::move(wayland_executor)}
{
}

void mf::WlShm::bind(wl_resource* new_wl_shm)
{
    new Shm{new_wl_shm, wayland_executor};
}

mf::Shm::Shm(wl_resource* resource, std::shared_ptr<Executor> wayland_executor)
    : wayland::Shm(resource, Version<1>{}),
      wayland_executor{std::move(wayland_executor)}
{
    // TODO: send all the formats we support, beyond the mandatory ones.
    for (auto format : { Format::argb8888, Format::xrgb8888 })
    {
        send_format_event(format);
    }
}

void mf::Shm::create_pool(wl_resource* id, Fd fd, int32_t size)
{
    new ShmPool{id, wayland_executor, fd, size};
}
