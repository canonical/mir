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

#include <mir/fd.h>
#include <mir/geometry/size.h>
#include <mir/graphics/drm_formats.h>

#include <memory>
#include <vector>

namespace mir
{
class Executor;

namespace shm
{
class RWMappableRange;
class ReadWritePool;
}

namespace renderer::software
{
class RWMappable;
}

namespace frontend
{
class ShmBuffer : public wayland_rs::Buffer
{
public:
    ShmBuffer(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::BufferMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<shm::RWMappableRange> data,
        geometry::Size size,
        geometry::Stride stride,
        graphics::DRMFormat format);

    auto data() -> std::shared_ptr<renderer::software::RWMappable>;

private:
    wayland_rs::Weak<ShmBuffer> const weak_me;
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shm::RWMappableRange> const data_;
    geometry::Size const size_;
    geometry::Stride const stride_;
    graphics::DRMFormat const format_;
};

class ShmPool : public wayland_rs::ShmPool
{
public:
    ShmPool(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::ShmPoolMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<std::vector<graphics::DRMFormat> const> supported_formats,
        Fd backing_store,
        int32_t claimed_size);

    auto create_buffer(
        int32_t offset,
        int32_t width, int32_t height,
        int32_t stride,
        uint32_t format,
        rust::Box<wayland_rs::BufferMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::Buffer> override;

    void resize(int32_t new_size) override;

private:
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<std::vector<graphics::DRMFormat> const> const supported_formats;
    std::shared_ptr<shm::ReadWritePool> const backing_store;
};

class Shm : public wayland_rs::Shm
{
public:
    Shm(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::ShmMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<std::vector<graphics::DRMFormat> const> supported_formats);

    auto create_pool(
        int32_t fd,
        int32_t size,
        rust::Box<wayland_rs::ShmPoolMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::ShmPool> override;

private:
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<std::vector<graphics::DRMFormat> const> const supported_formats;
};
}
}

#endif // MIR_FRONTEND_SHM_H
