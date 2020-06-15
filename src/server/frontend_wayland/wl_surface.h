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

#include "wayland_wrapper.h"

#include "wl_surface_role.h"

#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "mir/geometry/point.h"

#include <vector>
#include <map>

namespace mir
{
class Executor;

namespace graphics
{
class WaylandAllocator;
}
namespace scene
{
class Session;
}
namespace shell
{
struct StreamSpecification;
}
namespace geometry
{
class Rectangle;
}
namespace compositor
{
class BufferStream;
}
namespace frontend
{
class WlSurface;
class WlSubsurface;

struct WlSurfaceState
{
    class Callback : public wayland::Callback
    {
    public:
        Callback(wl_resource* new_resource);
        std::shared_ptr<bool> destroyed;
    };

    // if you add variables, don't forget to update this
    void update_from(WlSurfaceState const& source);

    void invalidate_surface_data() const { surface_data_invalidated = true; }

    bool surface_data_needs_refresh() const;

    // NOTE: buffer can be both nullopt and nullptr (I know, sounds dumb, but bare with me)
    // if it's nullopt, there is not a new buffer and no value should be copied to current state
    // if it's nullptr, there is a new buffer and it is a null buffer, which should replace the current buffer
    std::experimental::optional<wl_resource*> buffer;

    std::experimental::optional<int> scale;
    std::experimental::optional<geometry::Displacement> offset;
    std::experimental::optional<std::experimental::optional<std::vector<geometry::Rectangle>>> input_shape;
    std::vector<std::shared_ptr<Callback>> frame_callbacks;

private:
    // only set to true if invalidate_surface_data() is called
    // surface_data_needs_refresh() returns true if this is true, or if other things are changed which mandate a refresh
    // is marked mutable so invalidate_surface_data() can be const and be called from a const reference
    // (this is the only thing we need to modify from the const reference)
    bool mutable surface_data_invalidated{false};
};

class NullWlSurfaceRole : public WlSurfaceRole
{
public:
    NullWlSurfaceRole(WlSurface* surface);
    auto scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>> override;
    void refresh_surface_data_now() override;
    void commit(WlSurfaceState const& state) override;
    void destroy() override;

private:
    WlSurface* const surface;
};

class WlSurface : public wayland::Surface
{
public:
    WlSurface(wl_resource* new_resource,
              std::shared_ptr<mir::Executor> const& executor,
              std::shared_ptr<mir::graphics::WaylandAllocator> const& allocator);

    ~WlSurface();

    geometry::Displacement offset() const { return offset_; }
    geometry::Displacement total_offset() const { return offset_ + role->total_offset(); }
    std::experimental::optional<geometry::Size> buffer_size() const { return buffer_size_; }
    bool synchronized() const;
    auto subsurface_at(geometry::Point point) -> std::experimental::optional<WlSurface*>;
    wl_resource* raw_resource() const { return resource; }
    auto scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>>;

    void set_role(WlSurfaceRole* role_);
    void clear_role();
    void set_pending_offset(std::experimental::optional<geometry::Displacement> const& offset);
    void add_subsurface(WlSubsurface* child);
    void remove_subsurface(WlSubsurface* child);
    void refresh_surface_data_now();
    void pending_invalidate_surface_data() { pending.invalidate_surface_data(); }
    void populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                               std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                               geometry::Displacement const& parent_offset) const;
    void commit(WlSurfaceState const& state);
    void add_destroy_listener(void const* key, std::function<void()> listener);
    void remove_destroy_listener(void const* key);

    std::shared_ptr<scene::Session> const session;
    std::shared_ptr<compositor::BufferStream> const stream;

    static WlSurface* from(wl_resource* resource);

private:
    std::shared_ptr<mir::graphics::WaylandAllocator> const allocator;
    std::shared_ptr<mir::Executor> const executor;

    NullWlSurfaceRole null_role;
    WlSurfaceRole* role;
    std::vector<WlSubsurface*> children; // ordering is from bottom to top

    WlSurfaceState pending;
    geometry::Displacement offset_;
    std::experimental::optional<geometry::Size> buffer_size_;
    std::vector<std::shared_ptr<WlSurfaceState::Callback>> frame_callbacks;
    std::experimental::optional<std::vector<mir::geometry::Rectangle>> input_shape;
    std::map<void const*, std::function<void()>> destroy_listeners;

    void send_frame_callbacks();

    void destroy() override;
    void attach(std::experimental::optional<wl_resource*> const& buffer, int32_t x, int32_t y) override;
    void damage(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void frame(wl_resource* callback) override;
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
