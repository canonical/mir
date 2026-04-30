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

#include "wl_compositor.h"
#include "wl_surface.h"
#include "wl_region.h"

namespace mw = mir::wayland_rs;
namespace mf = mir::frontend;

mf::WlCompositor::WlCompositor(std::shared_ptr<WlClient> const& client,
    std::shared_ptr<mir::Executor> const& wayland_executor,
    std::shared_ptr<mir::Executor> const& frame_callback_executor,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    rust::Box<wayland_rs::WaylandEventLoopHandle> event_loop_handle)
    : client{client},
      wayland_executor(wayland_executor),
      frame_callback_executor(frame_callback_executor),
      allocator(allocator),
      event_loop_handle(std::move(event_loop_handle))
{
}

auto mf::WlCompositor::create_region() -> std::shared_ptr<wayland_rs::WlRegionImpl>
{
    return std::make_shared<WlRegion>();
}

auto mf::WlCompositor::create_surface() -> std::shared_ptr<wayland_rs::WlSurfaceImpl>
{
    return std::make_shared<WlSurface>(
        client,
        wayland_executor,
        frame_callback_executor,
        allocator,
        event_loop_handle->clone_box()
    );
}
