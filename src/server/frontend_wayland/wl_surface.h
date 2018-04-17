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

#include "wl_surface_role.h"

#include "mir/frontend/buffer_stream_id.h"
#include "mir/frontend/surface_id.h"

#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "mir/geometry/point.h"

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
namespace geometry
{
class Rectangle;
}

namespace frontend
{
class BufferStream;
class Session;
class WlSubsurface;

struct WlSurfaceState
{
    struct Callback
    {
        wl_resource* resource;
        std::shared_ptr<bool> destroyed;
    };

    // if you add variables, don't forget to update this
    void update_from(WlSurfaceState const& source);

    void invalidate_surface_data() { surface_data_invalidated = true; }

    bool surface_data_needs_refresh() const;

    // NOTE: buffer can be both nullopt and nullptr (I know, sounds dumb, but bare with me)
    // if it's nullopt, there is not a new buffer and no value should be copied to current state
    // if it's nullptr, there is a new buffer and it is a null buffer, which should replace the current buffer
    std::experimental::optional<wl_resource*> buffer;

    std::experimental::optional<geometry::Displacement> offset;
    std::experimental::optional<std::experimental::optional<std::vector<geometry::Rectangle>>> input_shape;
    std::vector<Callback> frame_callbacks;

private:
    // only set to true if invalidate_surface_data() is called
    // surface_data_needs_refresh() returns true if this is true, or if other things are changed which mandate a refresh
    bool surface_data_invalidated{false};
};

class NullWlSurfaceRole : public WlSurfaceRole
{
public:
    NullWlSurfaceRole(WlSurface* surface);
    SurfaceId surface_id() const override;
    void refresh_surface_data_now() override;
    void commit(WlSurfaceState const& state) override;
    void visiblity(bool /*visible*/) override;
    void destroy() override;

private:
    WlSurface* const surface;
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
    geometry::Displacement offset() const { return offset_; }
    geometry::Size buffer_size() const { return buffer_size_; }
    bool synchronized() const;
    std::pair<geometry::Point, wl_resource*> transform_point(geometry::Point point) const;
    wl_resource* raw_resource() { return resource; }
    mir::frontend::SurfaceId surface_id() const;

    void set_role(WlSurfaceRole* role_);
    void clear_role();
    void set_pending_offset(geometry::Displacement const& offset) { pending.offset = offset; }
    std::unique_ptr<WlSurface, std::function<void(WlSurface*)>> add_child(WlSubsurface* child);
    void refresh_surface_data_now();
    void pending_invalidate_surface_data() { pending.invalidate_surface_data(); }
    void populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                               std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                               geometry::Displacement const& parent_offset) const;
    void commit(WlSurfaceState const& state);

    std::shared_ptr<mir::frontend::Session> const session;
    mir::frontend::BufferStreamId const stream_id;
    std::shared_ptr<mir::frontend::BufferStream> const stream;

    static WlSurface* from(wl_resource* resource);

private:
    std::shared_ptr<mir::graphics::WaylandAllocator> const allocator;
    std::shared_ptr<mir::Executor> const executor;

    NullWlSurfaceRole null_role;
    WlSurfaceRole* role;
    std::vector<WlSubsurface*> children;

    WlSurfaceState pending;
    geometry::Displacement offset_;
    geometry::Size buffer_size_;
    std::vector<WlSurfaceState::Callback> frame_callbacks;
    std::experimental::optional<std::vector<mir::geometry::Rectangle>> input_shape;
    std::shared_ptr<bool> const destroyed;

    void send_frame_callbacks();

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
