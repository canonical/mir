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

#include "fractional_scale_v1.h"
#include <mir/geometry/forward.h>
#include "wayland.h"
#include "weak.h"
#include "wl_surface_role.h"
#include "wl_client.h"

#include <mir/geometry/displacement.h>
#include <mir/geometry/size.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/rectangles.h>
#include <mir/shell/surface_specification.h>
#include "linux_drm_syncobj.h"

#include <vector>
#include <list>

namespace mir
{
class Executor;

namespace wayland_rs
{
struct WaylandEventLoopHandle;
struct FdWatchToken;
}

namespace graphics
{
class GraphicBufferAllocator;
class Buffer;
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
class Viewport;
class SyncTimeline;

struct WlSurfaceState
{
    class Callback : public wayland_rs::WlCallbackImpl
    {
    };

    // if you add variables, don't forget to update this
    void update_from(WlSurfaceState const& source);

    void invalidate_surface_data() const { surface_data_invalidated = true; }

    bool surface_data_needs_refresh() const;

    // NOTE: nullopt has a distinct meaning from the optional containing a null Weak here
    // nullopt: the current state should not be changed
    // null Weak: the current buffer, if any, should be cleared
    std::optional<wayland_rs::Weak<wayland_rs::WlBufferImpl>> buffer;

    shell::SurfaceSpecification surface_spec;
    std::optional<float> scale;
    std::optional<geometry::Displacement> offset;
    std::optional<std::optional<std::vector<geometry::Rectangle>>> input_shape;
    std::optional<geometry::Rectangles> opaque_region;
    std::optional<MirOrientation> orientation;
    std::optional<MirMirrorMode> mirror_mode;
    std::vector<wayland_rs::Weak<Callback>> frame_callbacks;
    wayland_rs::Weak<Viewport> viewport;

    std::optional<SyncPoint> release_fence;

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

class WlSurface : public wayland_rs::WlSurfaceImpl, public std::enable_shared_from_this<WlSurface>
{
public:
    WlSurface(std::shared_ptr<WlClient> const& client,
              std::shared_ptr<mir::Executor> const& wayland_executor,
              std::shared_ptr<mir::Executor> const& frame_callback_executor,
              std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
              rust::Box<wayland_rs::WaylandEventLoopHandle> event_loop_handle);

    ~WlSurface() override;

    using SceneSurfaceCreatedCallback = std::function<void(std::shared_ptr<scene::Surface>)>;

    geometry::Displacement offset() const { return offset_; }
    geometry::Displacement total_offset() const { return offset_ + role->total_offset(); }
    std::optional<geometry::Size> buffer_size() const { return buffer_size_; }
    bool synchronized() const;
    auto subsurface_at(geometry::Point point) -> std::optional<WlSurface*>;
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
    bool has_subsurface_with_surface(WlSurface* surface) const;
    enum class SubsurfacePlacement { above, below };

    void reorder_subsurface(WlSubsurface* child, WlSurface* sibling, SubsurfacePlacement placement);
    void refresh_surface_data_now();
    void pending_invalidate_surface_data() { pending.invalidate_surface_data(); }
    void populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                               std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                               geometry::Displacement const& parent_offset) const;
    void commit(WlSurfaceState const& state);
    auto confine_pointer_state() const -> MirPointerConfinementState;

    void set_fractional_scale(std::shared_ptr<FractionalScaleV1> const& fractional_scale);
    auto get_fractional_scale() const -> wayland_rs::Weak<FractionalScaleV1>;

    /**
     * Associate a viewport (buffer scale & crop metadata) with this surface
     *
     * \throws A std::logic_error if the surface already has a viewport associated
     */
    void associate_viewport(wayland_rs::Weak<Viewport> viewport);

    /**
     * Associate a DRM Syncobj timeline with this surface
     *
     * This associates explicit synchronisation fences with the surface's
     * buffer submissions.
     * See linux-drm-syncobj-v1 protocol for more details
     *
     * \throws A TimelineAlreadyAssociated if the surface already has a timeline associated
     */
    void associate_sync_timeline(wayland_rs::Weak<SyncTimeline> timeline);

    auto attach(wayland_rs::Weak<wayland_rs::WlBufferImpl> const& buffer, bool has_buffer, int32_t x, int32_t y) -> void override;
    void damage(int32_t x, int32_t y, int32_t width, int32_t height) override;
    auto frame() -> std::shared_ptr<wayland_rs::WlCallbackImpl> override;
    auto set_opaque_region(wayland_rs::Weak<wayland_rs::WlRegionImpl> const& region, bool has_region) -> void override;
    auto set_input_region(wayland_rs::Weak<wayland_rs::WlRegionImpl> const& region, bool has_region) -> void override;
    void commit() override;
    void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) override;
    auto set_buffer_transform(uint32_t transform) -> void override;
    void set_buffer_scale(int32_t scale) override;
    void offset(int32_t x, int32_t y) override;

    class TimelineAlreadyAssociated : public std::logic_error
    {
    public:
        TimelineAlreadyAssociated()
            : std::logic_error("Surface already has a sync timeline associated")
        {
        }
    };

    std::shared_ptr<scene::Session> const session;
    std::shared_ptr<compositor::BufferStream> const stream;

    static WlSurface* from(WlSurfaceImpl* impl);

private:
    std::weak_ptr<WlClient> client;
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<mir::Executor> const frame_callback_executor;

    NullWlSurfaceRole null_role;
    WlSurfaceRole* role;
    std::vector<WlSubsurface*> children; // ordering is from bottom to top
    ptrdiff_t parent_z_index{0}; // index in children where parent surface renders (subsurfaces before this are below parent)
    /* We might need to resubmit the current buffer, but with different metadata
     * For example: if a client commits a wl_surface.buffer_scale() without attaching
     * a new buffer, we need to generate a new frame with the same content, but at the
     * new scale.
     *
     * To simplify other interfaces, keep a reference to the current content so we can
     * hide this, here, in the frontend.
     */
    std::shared_ptr<graphics::Buffer> current_buffer;

    /* State for when a buffer update is waiting for a client fence to signal
     */
    struct PendingBufferState;
    struct BufferReadyCallback;
    std::unique_ptr<PendingBufferState> pending_context;
    std::optional<rust::Box<wayland_rs::FdWatchToken>> buffer_ready_token;

    rust::Box<wayland_rs::WaylandEventLoopHandle> event_loop_handle;

    static void complete_commit(WlSurface* surf);

    WlSurfaceState pending;
    // Subsurface z-order state (plus a nullptr representing the parent surface)
    // This is separate from WlSurfaceState because it must be applied immediately on commit,
    // even if the parent surface is a synchronized subsurface (per Wayland spec)
    std::optional<std::list<WlSubsurface*>> pending_surface_order;
    geometry::Displacement offset_;
    float scale{1};
    std::optional<geometry::Size> buffer_size_;

    using CallbackList = std::vector<wayland_rs::Weak<WlSurfaceState::Callback>>;
    CallbackList frame_callbacks;
    CallbackList heartbeat_quirk_frame_callbacks;
    std::optional<std::vector<mir::geometry::Rectangle>> input_shape;
    std::vector<SceneSurfaceCreatedCallback> scene_surface_created_callbacks;
    wayland_rs::Weak<Viewport> viewport;
    wayland_rs::Weak<FractionalScaleV1> fractional_scale;
    wayland_rs::Weak<SyncTimeline> sync_timeline;

    void send_frame_callbacks(CallbackList& list);

    std::optional<geometry::Rectangles> opaque_region_;
};
}
}

#endif // MIR_FRONTEND_WL_SURFACE_H
