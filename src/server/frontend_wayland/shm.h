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

#ifndef MIR_FRONTEND_SHM_H
#define MIR_FRONTEND_SHM_H

#include "wayland.h"
#include "weak.h"
#include <mir/graphics/drm_formats.h>
#include <mir/geometry/size.h>
#include <mir/fd.h>

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

class ShmBuffer : public wayland_rs::WlBufferImpl
{
public:
    auto data() -> std::shared_ptr<renderer::software::RWMappable>;

    static auto from(WlBufferImpl* impl) -> ShmBuffer*;
private:
    friend class ShmPool;
    ShmBuffer(
        struct wl_resource* resource,
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<shm::RWMappableRange> data,
        geometry::Size size,
        geometry::Stride stride,
        graphics::DRMFormat format);

    wayland_rs::Weak<ShmBuffer> const weak_me;
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shm::RWMappableRange> const data_;
    geometry::Size const size_;
    geometry::Stride const stride_;
    graphics::DRMFormat const format_;
};

class ShmPool : public wayland_rs::WlShmPoolImpl
{
public:
    ShmPool(
        std::shared_ptr<Executor> wayland_executor,
        Fd backing_store,
        int32_t claimed_size);

    auto create_buffer(int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format)
        -> std::shared_ptr<wayland_rs::WlBufferImpl> override;

    void resize(int32_t new_size) override;

private:
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shm::ReadWritePool> const backing_store;
};

class Shm : public wayland_rs::WlShmImpl
{
public:
    explicit Shm(std::shared_ptr<Executor> wayland_executor);
    auto create_pool(int32_t fd, int32_t size) -> std::shared_ptr<wayland_rs::WlShmPoolImpl> override;

private:
    std::shared_ptr<Executor> const wayland_executor;
};
}

#endif  // MIR_FRONTEND_SHM_H