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

#include <memory>

namespace mir
{
class Executor;

namespace graphics
{
class GraphicBufferAllocator;
}

namespace wayland
{
class WaylandServer;
}

namespace frontend
{
class WlCompositor : public wayland::Compositor
{
public:
    WlCompositor(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::CompositorMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<Executor> const& frame_callback_executor,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        wayland::WaylandServer& server);

private:
    auto create_surface(
        rust::Box<wayland::SurfaceMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::Surface> override;
    auto create_region(
        rust::Box<wayland::RegionMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::Region> override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<Executor> const frame_callback_executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    wayland::WaylandServer& server;
};
}
}

#endif // MIR_FRONTEND_WL_COMPOSITOR_H
