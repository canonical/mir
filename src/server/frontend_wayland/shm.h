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

#include "mir/geometry/size.h"
#include "mir/wayland/weak.h"
#include "wayland_wrapper.h"
#include "mir/graphics/drm_formats.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <system_error>

namespace mir
{
class Executor;
}

namespace mir::shm
{
class RWMappableRange;
class ReadWritePool;
}

namespace mir::renderer::software
{
class RWMappable;
}

namespace mir::frontend
{
class Shm;
class ShmPool;

class ShmBuffer : public wayland::Buffer
{
public:
    auto data() -> std::shared_ptr<renderer::software::RWMappable>;

    static auto from(wl_resource* resource) -> ShmBuffer*;
private:
    friend class ShmPool;
    ShmBuffer(
        struct wl_resource* resource,
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<shm::RWMappableRange> data,
        geometry::Size size,
        geometry::Stride stride,
        graphics::DRMFormat format);

    wayland::Weak<ShmBuffer> const weak_me;
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shm::RWMappableRange> const data_;
    geometry::Size const size_;
    geometry::Stride const stride_;
    graphics::DRMFormat const format_;
};

class ShmPool : public wayland::ShmPool
{
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
private:
    friend class WlShm;
    Shm(struct wl_resource* resource, std::shared_ptr<Executor> wayland_executor);

    void create_pool(struct wl_resource* id, Fd fd, int32_t size) override;

    std::shared_ptr<Executor> const wayland_executor;
};

class WlShm : public wayland::Shm::Global
{
public:
    WlShm(wl_display* display, std::shared_ptr<Executor> wayland_executor);

private:
    void bind(wl_resource* new_wl_shm) override;

    std::shared_ptr<Executor> const wayland_executor;
};
}
