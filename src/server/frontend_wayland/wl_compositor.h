/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_WL_COMPOSITOR_H
#define MIR_FRONTEND_WL_COMPOSITOR_H

#include "wayland.h"
#include "wl_client.h"
#include <map>

namespace mir
{
class Executor;
namespace graphics
{
class GraphicBufferAllocator;
}
namespace frontend
{
class WlSurface;
class WlCompositor;
class WlCompositorGlobal
{
public:
    WlCompositorGlobal(
        std::shared_ptr<mir::Executor> const& wayland_executor,
        std::shared_ptr<mir::Executor> const& frame_callback_executor,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator);

    void on_surface_created(std::shared_ptr<WlSurface> const& surface, std::function<void(WlSurface*)> const& callback);
private:
    friend WlCompositor;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<mir::Executor> const frame_callback_executor;
    std::map<std::shared_ptr<WlSurface>, std::vector<std::function<void(WlSurface*)>>> surface_callbacks;
};

class WlCompositor : public wayland_rs::WlCompositorImpl
{
public:
    WlCompositor(
        WlCompositorGlobal* global,
        std::shared_ptr<WlClient> const& client,
        std::shared_ptr<mir::Executor> const& wayland_executor,
        std::shared_ptr<mir::Executor> const& frame_callback_executor,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        rust::Box<wayland_rs::WaylandEventLoopHandle> event_loop_handle);

    auto create_surface() -> std::shared_ptr<wayland_rs::WlSurfaceImpl> override;
    auto create_region() -> std::shared_ptr<wayland_rs::WlRegionImpl> override;

private:
    WlCompositorGlobal* global;
    std::shared_ptr<WlClient> client;
    std::shared_ptr<mir::Executor> wayland_executor;
    std::shared_ptr<mir::Executor> frame_callback_executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> allocator;
    rust::Box<wayland_rs::WaylandEventLoopHandle> event_loop_handle;
};
}
}

#endif //MIR_FRONTEND_WL_COMPOSITOR_H
