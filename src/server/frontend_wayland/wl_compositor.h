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

namespace mir
{
class Executor;
namespace graphics
{
class GraphicBufferAllocator;
}
namespace frontend
{
class WlCompositor : public wayland_rs::WlCompositorImpl
{
public:
    WlCompositor(std::shared_ptr<WlClient> const& client,
        std::shared_ptr<mir::Executor> const& wayland_executor,
        std::shared_ptr<mir::Executor> const& frame_callback_executor,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        rust::Box<wayland_rs::WaylandEventLoopHandle> event_loop_handle);

    auto create_surface() -> std::shared_ptr<wayland_rs::WlSurfaceImpl> override;
    auto create_region() -> std::shared_ptr<wayland_rs::WlRegionImpl> override;

private:
    std::shared_ptr<WlClient> client;
    std::shared_ptr<mir::Executor> wayland_executor;
    std::shared_ptr<mir::Executor> frame_callback_executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> allocator;
    rust::Box<wayland_rs::WaylandEventLoopHandle> event_loop_handle;
};
}
}

#endif //MIR_FRONTEND_WL_COMPOSITOR_H
