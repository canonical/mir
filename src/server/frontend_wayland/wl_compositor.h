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

namespace wayland_rs
{
class WaylandServer;
}

namespace frontend
{
class WlCompositor : public wayland_rs::Compositor
{
public:
    WlCompositor(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::CompositorMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<Executor> const& frame_callback_executor,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        wayland_rs::WaylandServer& server);

private:
    auto create_surface(
        rust::Box<wayland_rs::SurfaceMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::Surface> override;
    auto create_region(
        rust::Box<wayland_rs::RegionMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::Region> override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<Executor> const frame_callback_executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    wayland_rs::WaylandServer& server;
};
}
}

#endif // MIR_FRONTEND_WL_COMPOSITOR_H
