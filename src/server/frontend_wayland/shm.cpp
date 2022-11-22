#include "shm.h"
#include "mir/graphics/drm_formats.h"
#include "shm_backing.h"

#include "mir/wayland/weak.h"
#include "mir/executor.h"

#include <drm/drm_fourcc.h>
#include <boost/throw_exception.hpp>
#include <wayland-server-protocol.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;

mf::ShmBuffer::ShmBuffer(
    struct wl_resource* resource,
    std::shared_ptr<mir::Executor> wayland_executor,
    std::unique_ptr<RWMappableRange> data,
    geometry::Size size,
    geometry::Stride stride,
    graphics::DRMFormat format)
    : Buffer{resource, Version<1>{}},
      weak_me{wayland::make_weak(this)},
      wayland_executor{std::move(wayland_executor)},
      data{std::move(data)},
      size_{std::move(size)},
      stride_{stride},
      format_{format}
{
}

auto mf::ShmBuffer::format() const -> MirPixelFormat
{
    auto mir_format = format_.as_mir_format();
    if (mir_format)
    {
        return *mir_format;
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"ShmBuffer has a format unrepresentable as MirPixelFormat"}));
}

auto mf::ShmBuffer::stride() const -> geometry::Stride
{
    return stride_;
}

auto mf::ShmBuffer::size() const -> geometry::Size
{
    return size_;
}

namespace
{
template<typename T>
class ErrorNotifyingMapping : public mir::renderer::software::Mapping<T>
{
public:
    ErrorNotifyingMapping(
        mf::ShmBuffer const& parent,
        std::unique_ptr<mir::Mapping<std::conditional_t<std::is_const_v<T>, std::byte const, std::byte>>> mapping,
        std::function<void()> on_error)
        : parent{parent},
          mapping{std::move(mapping)},
          on_error{std::move(on_error)}
    {
    }

    ~ErrorNotifyingMapping()
    {
        if (mapping->access_fault())
        {
            on_error();
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
    mf::ShmBuffer const& parent;
    std::unique_ptr<mir::Mapping<std::conditional_t<std::is_const_v<T>, std::byte const, std::byte>>> const mapping;
    std::function<void()> on_error;
};
}



auto mf::ShmBuffer::map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>>
{
    return std::make_unique<ErrorNotifyingMapping<unsigned char>>(
        *this,
        data->map_rw(),
        [weak_buffer = weak_me, executor = wayland_executor]()
        {
            executor->spawn(
                [weak_buffer]()
                {
                    if (weak_buffer)
                    {
                        wl_resource_post_error(
                            weak_buffer.value().resource,
                            Shm::Error::invalid_fd,
                            "Error accessing SHM buffer");
                    }
                });
        });
}

auto mf::ShmBuffer::map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>>
{
    return std::make_unique<ErrorNotifyingMapping<unsigned char const>>(
        *this,
        data->map_ro(),
        [weak_buffer = weak_me, executor = wayland_executor]()
        {
            executor->spawn(
                [weak_buffer]()
                {
                    if (weak_buffer)
                    {
                        wl_resource_post_error(
                            weak_buffer.value().resource,
                            Shm::Error::invalid_fd,
                            "Error accessing SHM buffer");
                    }
                });
        });
}

auto mf::ShmBuffer::map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>>
{
    return std::make_unique<ErrorNotifyingMapping<unsigned char>>(
        *this,
        data->map_wo(),
        [weak_buffer = weak_me, executor = wayland_executor]()
        {
            executor->spawn(
                [weak_buffer]()
                {
                    if (weak_buffer)
                    {
                        wl_resource_post_error(
                            weak_buffer.value().resource,
                            Shm::Error::invalid_fd,
                            "Error accessing SHM buffer");
                    }
                });
        });
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
        mg::DRMFormat{format}
    };
}

void mf::ShmPool::resize(int32_t new_size)
{
    backing_store->resize(new_size);
}

mf::Shm::Global::Global(wl_display* display, std::shared_ptr<Executor> wayland_executor)
    : wayland::Shm::Global(display, Version<1>{}),
      wayland_executor{std::move(wayland_executor)}
{
}

void mf::Shm::Global::bind(wl_resource* new_wl_shm)
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
