/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_GRAPHICS_WAYLAND_ALLOCATOR_H_
#define MIR_PLATFORM_GRAPHICS_WAYLAND_ALLOCATOR_H_

#include <memory>
#include <functional>

#include <wayland-server-core.h>

namespace mir
{
class Executor;

namespace graphics
{
class Buffer;

class WaylandAllocator
{
public:
    WaylandAllocator();
    virtual ~WaylandAllocator();

    WaylandAllocator(WaylandAllocator const&) = delete;
    WaylandAllocator& operator=(WaylandAllocator const&) = delete;

    virtual void bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor) = 0;
    virtual std::shared_ptr<Buffer> buffer_from_resource(
        wl_resource* buffer,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) = 0;
    virtual auto buffer_from_shm(
        wl_resource* buffer,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::function<void()>&& on_consumed) -> std::shared_ptr<Buffer> = 0;
};
}
}

#endif //MIR_PLATFORM_GRAPHICS_WAYLAND_ALLOCATOR_H_
