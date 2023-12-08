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

#ifndef MIR_FRONTEND_WL_SURFACE_H
#define MIR_FRONTEND_WL_SURFACE_H

#include "wayland_wrapper.h"
#include "mir/wayland/weak.h"

#include "wl_surface_role.h"

#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "mir/geometry/point.h"
#include "mir/geometry/rectangle.h"
#include "mir/shell/surface_specification.h"

#include <vector>
#include <map>

namespace mir
{
class Executor;

namespace graphics
{
class GraphicBufferAllocator;
}
namespace scene
{
class Session;
}
namespace shell
{
struct StreamSpecification;
}
namespace compositor
{
class BufferStream;
}
namespace frontend
{
class WlSurface;
class WlSubsurface;
class ResourceLifetimeTracker;

struct WlSurfaceState
{
    class Callback : public wayland::Callback
    {
    public:
        Callback(wl_resource* new_resource);
    };

    // if you add variables, don't forget to update this
    void update_from(WlSurfaceState const& source);

    void invalidate_surface_data() const { surface_data_invalidated = true; }

    bool surface_data_needs_refresh() const;

    // NOTE: nullopt has a distinct meaning from the optional containing a null Weak here
    // nullopt: the current state should not be changed
    // null Weak: the current buffer, if any, should be cleared
    std::optional<wayland::Weak<ResourceLifetimeTracker>> buffer;

    shell::SurfaceSpecification surface_spec;
    std::optional<int> scale;
    std::optional<geometry::Displacement> offset;
    std::optional<std::optional<std::vector<geometry::Rectangle>>> input_shape;
    std::vector<wayland::Weak<Callback>> frame_callbacks;

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
    auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>> override;
    void refresh_surface_data_now() override;
    void commit(WlSurfaceState const& state) override;
    void surface_destroyed() override;

private:
    WlSurface* const surface;
};

class WlSurface : public wayland::Surface
{
public:
    WlSurface(wl_resource* new_resource,
              std::shared_ptr<mir::Executor> const& wayland_executor,
              std::shared_ptr<mir::Executor> const& frame_callback_executor,
              std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator);

    ~WlSurface();

    using SceneSurfaceCreatedCallback = std::function<void(std::shared_ptr<scene::Surface>)>;

    geometry::Displacement offset() const { return offset_; }
    geometry::Displacement total_offset() const { return offset_ + role->total_offset(); }
    std::optional<geometry::Size> buffer_size() const { return buffer_size_; }
    bool synchronized() const;
    auto subsurface_at(geometry::Point point) -> std::optional<WlSurface*>;
    wl_resource* raw_resource() const { return resource; }
    auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>;
    /// Callback is called immediately if the surface already has a scene::Surface, or else on the first commit where
    /// one exists
    void on_scene_surface_created(SceneSurfaceCreatedCallback&& callback);

    void update_surface_spec(shell::SurfaceSpecification const& spec);
    void set_role(WlSurfaceRole* role_);
    void clear_role();
    void set_pending_offset(std::optional<geometry::Displacement> const& offset);
    void add_subsurface(WlSubsurface* child);
    void remove_subsurface(WlSubsurface* child);
    void refresh_surface_data_now();
    void pending_invalidate_surface_data() { pending.invalidate_surface_data(); }
    void populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                               std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                               geometry::Displacement const& parent_offset) const;
    void commit(WlSurfaceState const& state);
    auto confine_pointer_state() const -> MirPointerConfinementState;

    std::shared_ptr<scene::Session> const session;
    std::shared_ptr<compositor::BufferStream> const stream;

    static WlSurface* from(wl_resource* resource);

private:
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<mir::Executor> const frame_callback_executor;

    NullWlSurfaceRole null_role;
    WlSurfaceRole* role;
    std::vector<WlSubsurface*> children; // ordering is from bottom to top

    WlSurfaceState pending;
    geometry::Displacement offset_;
    std::optional<geometry::Size> buffer_size_;
    std::vector<wayland::Weak<WlSurfaceState::Callback>> frame_callbacks;
    std::optional<std::vector<mir::geometry::Rectangle>> input_shape;
    std::vector<SceneSurfaceCreatedCallback> scene_surface_created_callbacks;

    void send_frame_callbacks();

    void attach(std::optional<wl_resource*> const& buffer, int32_t x, int32_t y) override;
    void damage(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void frame(wl_resource* callback) override;
    void set_opaque_region(std::optional<wl_resource*> const& region) override;
    void set_input_region(std::optional<wl_resource*> const& region) override;
    void commit() override;
    void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_buffer_transform(int32_t transform) override;
    void set_buffer_scale(int32_t scale) override;
};
}
}

#endif // MIR_FRONTEND_WL_SURFACE_H
