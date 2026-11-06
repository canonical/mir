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

#include "wayland.h"
#include "weak.h"

#include <mir/geometry/forward.h>
#include "protocol_error.h"

#include "wl_surface_role.h"

#include <mir/geometry/displacement.h>
#include <mir/geometry/size.h>
#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/rectangles.h>
#include <mir/shell/surface_specification.h>
#include "linux_drm_syncobj.h"

#include <atomic>
#include <vector>
#include <list>
#include <map>
#include <typeinfo>
#include <optional>

namespace mir
{
class Executor;

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
namespace wayland
{
class WaylandServer;
}
namespace frontend
{
class WlSurface;
class WlSubsurface;
class Viewport;
class SyncTimeline;
class FractionalScaleV1;
class ExtForeignBufferV1;

/// A mir_shell_v1 archetype assigned to a wl_surface. An archetype augments a role rather than
/// being a role itself, so it is tracked separately from WlSurfaceRole.
class WlSurfaceArchetype
{
public:
    virtual ~WlSurfaceArchetype() = default;

    /// Called when the surface is given a different archetype, or the surface goes away. The
    /// object is inert from this point on and must reject any further requests from the client.
    virtual void make_inert() = 0;

protected:
    WlSurfaceArchetype() = default;
    WlSurfaceArchetype(WlSurfaceArchetype const&) = delete;
    auto operator=(WlSurfaceArchetype const&) -> WlSurfaceArchetype& = delete;
};

struct WlSurfaceState
{
    class Callback : public wayland::Callback
    {
    public:
        Callback(
            std::shared_ptr<wayland::Client> client,
            rust::Box<wayland::CallbackMiddleware> instance,
            uint32_t object_id);
    };

    // if you add variables, don't forget to update this
    void update_from(WlSurfaceState const& source);

    void invalidate_surface_data() const { surface_data_invalidated = true; }

    bool surface_data_needs_refresh() const;

    // NOTE: nullopt has a distinct meaning from the optional containing a null Weak here
    // nullopt: the current state should not be changed
    // null Weak: the current buffer, if any, should be cleared
    std::optional<wayland::Weak<wayland::Buffer>> buffer;

    shell::SurfaceSpecification surface_spec;
    std::optional<float> scale;
    std::optional<geometry::Displacement> offset;
    std::optional<std::optional<std::vector<geometry::Rectangle>>> input_shape;
    std::optional<geometry::Rectangles> opaque_region;
    std::optional<MirOrientation> orientation;
    std::optional<MirMirrorMode> mirror_mode;
    std::vector<wayland::Weak<Callback>> frame_callbacks;
    wayland::Weak<Viewport> viewport;

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

class WlSurface : public wayland::Surface
{
public:
    WlSurface(std::shared_ptr<wayland::Client> client,
              rust::Box<wayland::SurfaceMiddleware> instance,
              uint32_t object_id,
              std::shared_ptr<mir::Executor> const& wayland_executor,
              std::shared_ptr<mir::Executor> const& frame_callback_executor,
              std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
              wayland::WaylandServer& server);

    ~WlSurface();

    using SceneSurfaceCreatedCallback = std::function<void(std::shared_ptr<scene::Surface>)>;

    geometry::Displacement offset() const { return offset_; }
    geometry::Displacement total_offset() const { return offset_ + role->total_offset(); }
    std::optional<geometry::Size> buffer_size() const { return buffer_size_; }
    /// Whether this surface has a buffer: either one already committed, or a non-null one attached and
    /// waiting for the next commit.
    bool has_buffer() const { return buffer_size_ || (pending.buffer && pending.buffer.value()); }

    /// Sets a function to call each time the client attaches a non-null buffer (wl_surface.attach).
    /// Attaching a null buffer does not call it.
    ///
    /// The hook runs before the buffer is stored. If it returns a ProtocolError, the attach is refused and that
    /// error is sent to the client. If it returns std::nullopt, the attach goes ahead.
    ///
    /// There is a single slot: setting a hook replaces any previous one. This surface owns the hook, so anything
    /// it captures must stay valid while it is set (hold it weakly otherwise).
    void set_attach_hook(std::function<std::optional<wayland::ProtocolError>()> hook) { attach_hook = std::move(hook); }
    bool synchronized() const;
    /// Whether the surface currently has a role assigned (e.g. subsurface, cursor,
    /// drag-and-drop icon, or a window role such as xdg_toplevel). Used to raise the
    /// appropriate protocol error when a client tries to assign a second role.
    bool has_role() const { return role != &null_role; }
    template<typename Role>
    auto can_set_role() const -> bool { return can_set_role_type(&typeid(Role)); }
    /// Whether this surface's role, if any, can be combined with a mir_shell_v1 archetype. An
    /// archetype augments a role that makes the surface a window the compositor manages; roles
    /// such as wl_subsurface and cursor have semantics inconsistent with an archetype.
    auto role_accepts_archetype() const -> bool;
    /// Whether a mir_shell_v1 archetype is currently assigned to this surface.
    auto has_archetype() const -> bool { return archetype; }
    /// Assigns an archetype, making any previously assigned archetype object inert.
    void set_archetype(WlSurfaceArchetype* archetype);
    /// Clears the archetype if it is the one currently assigned; a no-op otherwise, as an inert
    /// archetype object may outlive its replacement being assigned.
    void clear_archetype(WlSurfaceArchetype const* archetype);
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

    /// Called by the [ExtForeignBufferV1] that this surface is displaying when the compositor refreshes
    /// its content, so that the new content reaches the scene without the client re-committing.
    void advise_foreign_buffer_content_changed(std::shared_ptr<graphics::Buffer> buffer);

    void set_fractional_scale(FractionalScaleV1* fractional_scale);
    auto get_fractional_scale() const -> wayland::Weak<FractionalScaleV1>;

    /**
     * Associate a viewport (buffer scale & crop metadata) with this surface
     *
     * \throws A std::logic_error if the surface already has a viewport associated
     */
    void associate_viewport(wayland::Weak<Viewport> viewport);

    /**
     * Associate a DRM Syncobj timeline with this surface
     *
     * This associates explicit synchronisation fences with the surface's
     * buffer submissions.
     * See linux-drm-syncobj-v1 protocol for more details
     *
     * \throws A TimelineAlreadyAssociated if the surface already has a timeline associated
     */
    void associate_sync_timeline(wayland::Weak<SyncTimeline> timeline);

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

private:
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<mir::Executor> const frame_callback_executor;
    wayland::WaylandServer& server;

    NullWlSurfaceRole null_role;
    WlSurfaceRole* role;
    std::type_info const* previous_role_type;
    WlSurfaceArchetype* archetype{nullptr};
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

    /// Set while the surface is displaying a foreign buffer, so pushed content updates can be
    /// ignored once the client attaches something else.
    wayland::Weak<ExtForeignBufferV1> current_foreign_buffer;

    /// Submits current_buffer to the stream. Returns whether the logical size changed.
    auto submit_current_buffer() -> bool;

    /* State for when a buffer update is waiting for a client fence to signal.
     *
     * WaylandServer::register_fd_ready_listener is one-shot and non-cancellable,
     * so a previously-registered wait cannot be removed. Instead each wait is
     * tagged with buffer_ready_generation; when a newer commit supersedes an
     * in-flight wait we bump the generation (and signal awaiting_release so the
     * client may reuse the buffer), and the stale listener no-ops when it fires.
     */
    friend class BufferReadyNotifier;
    uint64_t buffer_ready_generation{0};
    bool awaiting_buffer{false};
    std::optional<SyncPoint> awaiting_release;

    static void complete_commit(WlSurface* surf);

    WlSurfaceState pending;
    // Subsurface z-order state (plus a nullptr representing the parent surface)
    // This is separate from WlSurfaceState because it must be applied immediately on commit,
    // even if the parent surface is a synchronized subsurface (per Wayland spec)
    std::optional<std::list<WlSubsurface*>> pending_surface_order;
    geometry::Displacement offset_;
    float scale{1};
    std::optional<geometry::Size> buffer_size_;

    using CallbackList = std::vector<wayland::Weak<WlSurfaceState::Callback>>;
    CallbackList frame_callbacks;
    CallbackList heartbeat_quirk_frame_callbacks;
    std::optional<std::vector<mir::geometry::Rectangle>> input_shape;
    std::vector<SceneSurfaceCreatedCallback> scene_surface_created_callbacks;
    std::function<std::optional<wayland::ProtocolError>()> attach_hook{
        []() -> std::optional<wayland::ProtocolError> { return std::nullopt; }};
    wayland::Weak<Viewport> viewport;
    wayland::Weak<FractionalScaleV1> fractional_scale;
    wayland::Weak<SyncTimeline> sync_timeline;

    void send_frame_callbacks(CallbackList& list);
    auto can_set_role_type(std::type_info const* role_) const -> bool;

    void attach(std::optional<wayland::Weak<wayland::Buffer>> const& buffer, int32_t x, int32_t y) override;
    void damage(int32_t x, int32_t y, int32_t width, int32_t height) override;
    auto frame(rust::Box<wayland::CallbackMiddleware> child_instance, uint32_t child_object_id)
        -> std::shared_ptr<wayland::Callback> override;
    void set_opaque_region(std::optional<wayland::Weak<wayland::Region>> const& region) override;
    void set_input_region(std::optional<wayland::Weak<wayland::Region>> const& region) override;
    void commit() override;
    void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_buffer_transform(uint32_t transform) override;
    void set_buffer_scale(int32_t scale) override;
    void offset(int32_t x, int32_t y) override;

    std::optional<geometry::Rectangles> opaque_region_;
};

/// Non-owning shared_ptr aliasing a live WlSurface, for synchronous wayland_rs
/// sender calls (send_enter/leave/down) that take std::shared_ptr<Surface>.
/// The returned pointer MUST NOT outlive the WlSurface.
inline auto as_surface_ptr(WlSurface* surface) -> std::shared_ptr<wayland::Surface>
{
    return surface ? std::shared_ptr<wayland::Surface>{std::shared_ptr<void>{}, surface} : nullptr;
}
}
}

#endif // MIR_FRONTEND_WL_SURFACE_H
