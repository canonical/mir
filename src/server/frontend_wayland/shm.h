#include "mir/wayland/weak.h"
#include "wayland_wrapper.h"
#include "mir/graphics/drm_formats.h"
#include "mir/renderer/sw/pixel_source.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <system_error>

namespace mir
{
class Executor;
class RWMappableRange;
}

namespace mir::shm
{
class ReadWritePool;
}

namespace mir::frontend
{
class Shm;
class ShmPool;

class ShmBuffer : public wayland::Buffer, public renderer::software::RWMappableBuffer
{
public:
    auto map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;
    auto map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>> override;
    auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override;

    auto format() const -> MirPixelFormat override;
    auto stride() const -> geometry::Stride override;
    auto size() const -> geometry::Size override;

    static auto from(wl_resource* resource) -> ShmBuffer*;
private:
    friend class ShmPool;
    ShmBuffer(
        struct wl_resource* resource,
        std::shared_ptr<Executor> wayland_executor,
        std::unique_ptr<RWMappableRange> data,
        geometry::Size size,
        geometry::Stride stride,
        graphics::DRMFormat format);

    wayland::Weak<ShmBuffer> const weak_me;
    std::shared_ptr<Executor> const wayland_executor;
    std::unique_ptr<RWMappableRange> const data;
    geometry::Size const size_;
    geometry::Stride const stride_;
    graphics::DRMFormat const format_;
};

class ShmPool : public wayland::ShmPool
{
public:
    ~ShmPool() override = default;
private:
    friend class Shm;
    ShmPool(
        struct wl_resource* resource,
        std::shared_ptr<Executor> wayland_executor,
        Fd backing_store,
        int32_t claimed_size);

    void create_buffer(
        struct wl_resource* id,
        int32_t offset,
        int32_t width, int32_t height,
        int32_t stride,
        uint32_t format) override;

    void resize(int32_t new_size) override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shm::ReadWritePool> const backing_store;
};

class Shm : public wayland::Shm
{
public:
    class Global : public wayland::Shm::Global
    {
    public:
        Global(wl_display* display, std::shared_ptr<Executor> wayland_executor);

    private:
        void bind(wl_resource* new_wl_shm) override;

        std::shared_ptr<Executor> const wayland_executor;
    };
private:
    friend class Global;
    Shm(struct wl_resource* resource, std::shared_ptr<Executor> wayland_executor);

    void create_pool(struct wl_resource* id, Fd fd, int32_t size) override;

    std::shared_ptr<Executor> const wayland_executor;
};
}
