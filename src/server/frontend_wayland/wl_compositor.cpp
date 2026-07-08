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

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace mg = mir::graphics;

mf::WlCompositor::WlCompositor(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::CompositorMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<Executor> const& frame_callback_executor,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    mw::WaylandServer& server)
    : mw::Compositor{std::move(client), std::move(instance), object_id},
      wayland_executor{wayland_executor},
      frame_callback_executor{frame_callback_executor},
      allocator{allocator},
      server{server}
{
}

auto mf::WlCompositor::create_surface(
    rust::Box<mw::SurfaceMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::Surface>
{
    return std::make_shared<WlSurface>(
        client,
        std::move(child_instance),
        child_object_id,
        wayland_executor,
        frame_callback_executor,
        allocator,
        server);
}

auto mf::WlCompositor::create_region(
    rust::Box<mw::RegionMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::Region>
{
    return std::make_shared<WlRegion>(
        client,
        std::move(child_instance),
        child_object_id);
}
