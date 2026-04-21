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

#include <mir/geometry/size.h>
#include "wayland.h"
#include <mir/graphics/drm_formats.h>
#include <mir/fd.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <system_error>

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

class ShmBuffer : public wayland_rs::WlBufferImpl, public std::enable_shared_from_this<ShmBuffer>
{
public:
    ShmBuffer(
        rust::Box<wayland_rs::WaylandClient> client,
        std::shared_ptr<shm::RWMappableRange> data,
        geometry::Size size,
        geometry::Stride stride,
        graphics::DRMFormat format);
    auto data() -> std::shared_ptr<renderer::software::RWMappable>;

private:

    std::shared_ptr<shm::RWMappableRange> const data_;
    geometry::Size const size_;
    geometry::Stride const stride_;
    graphics::DRMFormat const format_;
};

class ShmPool : public wayland_rs::WlShmPoolImpl
{
public:
    ShmPool(
        rust::Box<wayland_rs::WaylandClient> client,
        mir::Fd backing_store,
        int32_t claimed_size);

private:

    auto create_buffer(int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format) -> std::shared_ptr<wayland_rs::WlBufferImpl> override;

    void resize(int32_t new_size) override;

    std::shared_ptr<shm::ReadWritePool> const backing_store;
};

class Shm : public wayland_rs::WlShmImpl
{
public:
    explicit Shm(rust::Box<wayland_rs::WaylandClient> client);

private:
    auto create_pool(int32_t fd, int32_t size) -> std::shared_ptr<wayland_rs::WlShmPoolImpl> override;
};
}
