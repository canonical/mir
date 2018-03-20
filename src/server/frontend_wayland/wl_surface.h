/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_WL_SURFACE_H
#define MIR_FRONTEND_WL_SURFACE_H

#include "generated/wayland_wrapper.h"

#include "mir/frontend/buffer_stream_id.h"
#include "mir/frontend/surface_id.h"

#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"

#include <vector>

namespace mir
{
class Executor;

namespace graphics
{
class WaylandAllocator;
}
namespace shell
{
struct StreamSpecification;
}

namespace frontend
{
class BufferStream;
class Session;
class WlSurfaceRole;
class WlSubsurface;

struct WlSurfaceState
{
    // if you add variables, don't forget to update this
    void update_from(WlSurfaceState const& source);

    // NOTE: buffer can be both nullopt and nullptr (I know, sounds dumb, but bare with me)
    // if it's nullopt, there is not a new buffer and no value should be copied to current state
    // if it's nullptr, there is a new buffer and it is a null buffer, which should replace the current buffer
    std::experimental::optional<wl_resource*> buffer;

    std::experimental::optional<geometry::Displacement> buffer_offset;
    std::vector<wl_resource*> frame_callbacks;
};

class WlSurface : public wayland::Surface
{
public:
    WlSurface(wl_client* client,
              wl_resource* parent,
              uint32_t id,
              std::shared_ptr<mir::Executor> const& executor,
              std::shared_ptr<mir::graphics::WaylandAllocator> const& allocator);

    ~WlSurface();

    std::shared_ptr<bool> destroyed_flag() const { return destroyed; }
    geometry::Displacement buffer_offset() const { return buffer_offset_; }
    geometry::Size buffer_size() const { return buffer_size_; }
    bool synchronized() const;

    void set_role(WlSurfaceRole* role_);
    void set_buffer_offset(geometry::Displacement const& offset) { pending.buffer_offset = offset; }
    std::unique_ptr<WlSurface, std::function<void(WlSurface*)>> add_child(WlSubsurface* child);
    void invalidate_buffer_list();
    void populate_buffer_list(std::vector<shell::StreamSpecification>& buffers,
                              geometry::Displacement const& parent_offset) const;
    void commit(WlSurfaceState const& state);

    std::shared_ptr<mir::frontend::Session> const session;
    mir::frontend::BufferStreamId const stream_id;
    std::shared_ptr<mir::frontend::BufferStream> const stream;
    mir::frontend::SurfaceId surface_id;       // ID of any associated surface

    static WlSurface* from(wl_resource* resource);

private:
    std::shared_ptr<mir::graphics::WaylandAllocator> const allocator;
    std::shared_ptr<mir::Executor> const executor;

    WlSurfaceRole* role;
    std::vector<WlSubsurface*> children;

    WlSurfaceState pending;
    geometry::Displacement buffer_offset_;
    geometry::Size buffer_size_;
    std::shared_ptr<bool> const destroyed;

    void destroy() override;
    void attach(std::experimental::optional<wl_resource*> const& buffer, int32_t x, int32_t y) override;
    void damage(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void frame(uint32_t callback) override;
    void set_opaque_region(std::experimental::optional<wl_resource*> const& region) override;
    void set_input_region(std::experimental::optional<wl_resource*> const& region) override;
    void commit() override;
    void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_buffer_transform(int32_t transform) override;
    void set_buffer_scale(int32_t scale) override;
};
}
}

#endif // MIR_FRONTEND_WL_SURFACE_H
