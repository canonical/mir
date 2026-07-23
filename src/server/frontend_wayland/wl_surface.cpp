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

#include "wl_surface.h"
#include "output_manager.h"
#include "wp_fractional_scale_v1.h"
#include "wp_viewporter.h"
#include "wl_surface_role.h"
#include "wl_subcompositor.h"
#include "wl_region.h"
#include "shm.h"
#include "linux_drm_syncobj.h"
#include "zwp_linux_dmabuf_v1.h"

#include "wayland.h"
#include "weak.h"
#include "client.h"
#include "protocol_error.h"
#include "fd_ready_callback.h"
#include "wayland_rs/src/ffi.rs.h"

#include "wayland_frontend.tp.h"

#include <mir/graphics/buffer_properties.h>
#include <mir/scene/session.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/executor.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/scene/surface.h>
#include <mir/shell/surface_specification.h>
#include <mir/log.h>

#include <chrono>
#include <ranges>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland;
namespace msh = mir::shell;

// One-shot fd-ready listener that resumes a fence-gated commit once the client's
// acquire syncobj eventfd becomes readable. Owns the eventfd (calloop only borrows
// it) and the release SyncPoint. Tagged with the surface's generation so a stale
// wait — superseded by a newer commit — no-ops when it eventually fires.
namespace mir::frontend
{
class BufferReadyNotifier : public mw::FdReadyCallback
{
public:
    BufferReadyNotifier(
        mw::Weak<WlSurface> surf,
        uint64_t generation,
        mir::Fd eventfd,
        SyncPoint release)
        : surf{std::move(surf)},
          generation{generation},
          eventfd{std::move(eventfd)},
          release{std::move(release)}
    {
    }

    auto ready() -> void override
    {
        if (!surf)
            return;

        auto& self = surf.value();
        if (!self.awaiting_buffer || self.buffer_ready_generation != generation)
            return;

        self.awaiting_buffer = false;
        self.awaiting_release.reset();
        self.pending.release_fence = release;

        try
        {
            WlSurface::complete_commit(&self);
        }
        catch (mw::ProtocolError const& err)
        {
            self.post_error(err.code(), err.message());
        }
        catch (...)
        {
            mir::log(
                mir::logging::Severity::error,
                "WlSurface",
                std::current_exception(),
                "Error processing deferred Surface::commit()");
        }
    }

private:
    mw::Weak<WlSurface> const surf;
    uint64_t const generation;
    mir::Fd const eventfd;
    SyncPoint const release;
};
}

mf::WlSurfaceState::Callback::Callback(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::CallbackMiddleware> instance,
    uint32_t object_id)
    : mw::Callback{std::move(client), std::move(instance), object_id}
{
}

void mf::WlSurfaceState::update_from(WlSurfaceState const& source)
{
    if (source.buffer)
        buffer = source.buffer;

    if (source.scale)
        scale = source.scale;

    if (source.orientation)
        orientation = source.orientation;

    if (source.mirror_mode)
        mirror_mode = source.mirror_mode;

    if (source.offset)
        offset = source.offset;

    if (source.input_shape)
        input_shape = source.input_shape;

    frame_callbacks.insert(end(frame_callbacks),
                           begin(source.frame_callbacks),
                           end(source.frame_callbacks));

    if (source.viewport)
        viewport = source.viewport;

    if (source.surface_data_invalidated)
        surface_data_invalidated = true;

    if (source.release_fence)
        release_fence = source.release_fence;
}

bool mf::WlSurfaceState::surface_data_needs_refresh() const
{
    return offset ||
           input_shape ||
           surface_data_invalidated;
}

mf::WlSurface::WlSurface(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::SurfaceMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<Executor> const& frame_callback_executor,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    mw::WaylandServer& server)
    : Surface(std::move(client), std::move(instance), object_id),
        session{this->client->client_session()},
        stream{session->create_buffer_stream({{}, mir_pixel_format_invalid, graphics::BufferUsage::undefined})},
        allocator{allocator},
        wayland_executor{wayland_executor},
        frame_callback_executor{frame_callback_executor},
        server{server},
        null_role{this},
        role{&null_role}
{
}

mf::WlSurface::~WlSurface()
{
    // We can't use a function try block as we want to access `client`:
    // "Before any catch clauses of a function-try-block on a destructor are entered,
    // all bases and non-variant members have already been destroyed."
    try
    {
        // Destroy the buffer stream first, as surface_destroyed() may throw
        session->destroy_buffer_stream(stream);
        role->surface_destroyed();
    }
    catch (...)
    {
        mir::log(
            mir::logging::Severity::error,
            "WlSurface",
            std::current_exception(),
            "Error in WlSurface::~WlSurface()");
    }
}

bool mf::WlSurface::synchronized() const
{
    return role->synchronized();
}

auto mf::WlSurface::subsurface_at(geom::Point point) -> std::optional<WlSurface*>
{
    if (!buffer_size_)
    {
        // surface not mapped
        return std::nullopt;
    }
    point = point - offset_;
    // loop backwards so the first subsurface we find that accepts the input is the topmost one
    for (auto const& child : std::ranges::reverse_view(children))
    {
        if (auto result = child->subsurface_at(point))
            return result;
    }
    geom::Rectangle surface_rect = {geom::Point{}, buffer_size_.value_or(geom::Size{})};
    for (auto& rect : input_shape.value_or(std::vector<geom::Rectangle>{surface_rect}))
    {
        if (intersection_of(rect, surface_rect).contains(point))
            return this;
    }
    return std::nullopt;
}

auto mf::WlSurface::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    return role->scene_surface();
}

void mf::WlSurface::on_scene_surface_created(SceneSurfaceCreatedCallback&& callback)
{
    if (auto const surface = scene_surface(); surface && surface.value())
    {
        callback(surface.value());
    }
    else
    {
        scene_surface_created_callbacks.push_back(std::move(callback));
    }
}

void mf::WlSurface::set_role(WlSurfaceRole* role_)
{
    if (role != &null_role)
        BOOST_THROW_EXCEPTION(std::runtime_error("Surface already has a role"));
    role = role_;
}

void mf::WlSurface::clear_role()
{
    role = &null_role;
}

void mf::WlSurface::set_pending_offset(std::optional<geom::Displacement> const& offset)
{
    pending.offset = offset;
}

void mf::WlSurface::add_subsurface(WlSubsurface* child)
{
    if (std::find(children.begin(), children.end(), child) != children.end())
    {
        log_warning("Subsurface %p added to surface %p multiple times", static_cast<void*>(child), static_cast<void*>(this));
        return;
    }

    children.push_back(child);
}

void mf::WlSurface::remove_subsurface(WlSubsurface* child)
{
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        // Adjust parent_z_index if we're removing a child from before it
        if (it - children.begin() < parent_z_index)
        {
            parent_z_index--;
        }
        children.erase(it);
    }
}

bool mf::WlSurface::has_subsurface_with_surface(WlSurface* surface) const
{
    return std::any_of(
        children.begin(),
        children.end(),
        [surface](auto const* child)
        {
            WlSurface* child_surface = child->get_surface();
            return child_surface == surface || child_surface->has_subsurface_with_surface(surface);
        });
}

void mf::WlSurface::reorder_subsurface(WlSubsurface* child, WlSurface* sibling_surface, SubsurfacePlacement placement)
{
    if (!pending_surface_order)
    {
        std::list<WlSubsurface*> surfaces(children.begin(), children.begin()+parent_z_index);
        surfaces.push_back(nullptr); // Special entry marking the parent position
        surfaces.insert(surfaces.end(), children.begin()+parent_z_index, children.end());

        pending_surface_order = surfaces;
    }

    // Find the child in the pending order
    if (auto const child_pos = std::find(pending_surface_order->begin(), pending_surface_order->end(), child);
        child_pos == pending_surface_order->end())
    {
        log_warning(
            "Subsurface (wl_surface@%u) attempted to reorder but not found in parent's (wl_surface@%u) children list",
            child->get_surface()->object_id(),
            object_id());
        return;
    }
    else
    {
        pending_surface_order->erase(child_pos);
    }

    // If sibling is this surface (the parent) use nullptr to represent it
    auto const target = (sibling_surface != this) ? sibling_surface : nullptr;

    // Find which subsurface has this sibling surface
    auto sibling_it = std::find_if(
        pending_surface_order->begin(),
        pending_surface_order->end(),
        [target](auto const* s) { return s ? s->get_surface() == target : target == nullptr; });

    if (sibling_it == pending_surface_order->end())
    {
        log_warning(
            "Subsurface (wl_surface@%u) attempted to reorder relative to a sibling (wl_surface@%u) not found in parent's (wl_surface@%u) children list",
            child->get_surface()->object_id(),
            sibling_surface->object_id(),
            object_id());
        return;
    }

    if (placement == SubsurfacePlacement::above)
    {
        // Place just above sibling (after it in the list, higher z-order)
        pending_surface_order->insert(std::next(sibling_it), child);
    }
    else
    {
        // Place just below sibling (before it in the list, lower z-order)
        pending_surface_order->insert(sibling_it, child);
    }
}

void mf::WlSurface::refresh_surface_data_now()
{
    role->refresh_surface_data_now();
}

void mf::WlSurface::populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                                          std::vector<geom::Rectangle>& input_shape_accumulator,
                                          geometry::Displacement const& parent_offset) const
{
    auto const offset = parent_offset + offset_;
    std::span const children_below_parent{children.begin(), children.begin() + parent_z_index};
    std::span const children_above_parent{children.begin() + parent_z_index, children.end()};

    for (auto const& c : children_below_parent)
    {
        c->populate_surface_data(buffer_streams, input_shape_accumulator, offset);
    }

    buffer_streams.push_back(msh::StreamSpecification{stream, offset});
    geom::Rectangle surface_rect = {geom::Point{} + offset, buffer_size_.value_or(geom::Size{})};
    if (input_shape)
    {
        for (auto rect : input_shape.value())
        {
            rect.top_left = rect.top_left + offset;
            rect = intersection_of(rect, surface_rect); // clip to surface
            input_shape_accumulator.push_back(rect);
        }

        // If we have an explicity specified empty input shape all inport should be ignored
        // however, if we give Mir an empty vector it will use a default input shape
        // therefore we add a zero size rect to the vector
        // TODO: sort this whole mess out
        if (input_shape.value().empty())
        {
            input_shape_accumulator.push_back({{}, {}});
        }
    }
    else
    {
        input_shape_accumulator.push_back(surface_rect);
    }

    for (auto const& c : children_above_parent)
    {
        c->populate_surface_data(buffer_streams, input_shape_accumulator, offset);
    }
}

void mf::WlSurface::send_frame_callbacks(CallbackList& list)
{
    for (auto const& frame : list)
    {
        if (frame)
        {
            auto const timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch());
            frame.value().send_done_event(timestamp_ms.count());
            frame.value().destroy_and_delete();
        }
    }
    list.clear();
}

void mf::WlSurface::attach(std::optional<mw::Weak<mw::Buffer>> const& buffer, int32_t x, int32_t y)
{
    if (x != 0 || y != 0)
    {
        if (get_box()->version() >= 5)
        {
            throw mw::ProtocolError{
                object_id(),
                Error::invalid_offset,
                "Non-zero offset (%d, %d) given to wl_surface.attach, but since version 5 "
                "the offset must be set with wl_surface.offset instead",
                x, y};
        }

        mir::log_warning("Client requested unimplemented non-zero attach offset. Rendering will be incorrect.");
    }

    // pending.buffer engaged-with-null means "clear"; nullopt means "unchanged".
    // wl_surface.attach always changes the buffer, so always engage it here.
    pending.buffer = buffer ? *buffer : mw::Weak<mw::Buffer>{};
}

void mf::WlSurface::damage(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    // This isn't essential, but could enable optimizations
}

void mf::WlSurface::damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    // This isn't essential, but could enable optimizations
}

auto mf::WlSurface::frame(rust::Box<mw::CallbackMiddleware> child_instance, uint32_t child_object_id)
    -> std::shared_ptr<mw::Callback>
{
    auto callback = std::make_shared<WlSurfaceState::Callback>(client, std::move(child_instance), child_object_id);
    pending.frame_callbacks.push_back(mw::make_weak(callback.get()));
    return callback;
}

void mf::WlSurface::set_opaque_region(std::optional<mw::Weak<mw::Region>> const& region)
{
    pending.opaque_region = region.transform([](auto const& weak_region)
        {
            geom::Rectangles opaque_region;
            if (auto* wl_region = mw::Region::from<WlRegion>(weak_region))
            {
                for (auto const& subregion : wl_region->rectangle_vector())
                    opaque_region.add(subregion);
            }
            return opaque_region;
        });
}

void mf::WlSurface::set_input_region(std::optional<mw::Weak<mw::Region>> const& region)
{
    if (region)
    {
        // since pending.input_shape is an optional optional, this is needed
        std::vector<geom::Rectangle> shape;
        if (auto* wl_region = mw::Region::from<WlRegion>(region.value()))
            shape = wl_region->rectangle_vector();
        pending.input_shape = decltype(pending.input_shape)::value_type{std::move(shape)};
    }
    else
    {
        // set the inner optional to nullopt to indicate the input region should be updated, but with a null region
        pending.input_shape = decltype(pending.input_shape)::value_type{};
    }
}

void mf::WlSurface::commit(WlSurfaceState const& state)
{
    if (state.offset)
        offset_ = state.offset.value();

    if (state.input_shape)
        input_shape = state.input_shape.value();

    if (state.scale)
        scale = state.scale.value();

    if (state.viewport)
    {
        viewport = std::move(state.viewport);
    }

    bool needs_buffer_submission =
        state.scale ||                                               // If the scale has changed, or...
        state.viewport ||                                            // ...we've added a viewport, or...
        state.orientation ||                                         // ...we've changed orientation, or...
        state.mirror_mode ||                                         // ...we've change mirror mode, or...
        (viewport && viewport.value().changed_since_last_resolve()); // ...the viewport has changed...
                                                                     // ...then we'll need to submit a new frame, even if the client hasn't
                                                                     // attached a new buffer.

    if (role)
    {
        if (auto const scene_surface = role->scene_surface())
        {
            if (state.orientation)
                scene_surface.value()->set_orientation(state.orientation.value());
            if (state.mirror_mode)
                scene_surface.value()->set_mirror_mode(state.mirror_mode.value());
            if (state.opaque_region)
                scene_surface.value()->set_opaque_region(state.opaque_region.value());
        }
     }

    if (state.buffer)
    {
        // We're going to lose the value of state, so copy the frame_callbacks first. We have to maintain a list of
        // callbacks in wl_surface because if a client commits multiple times before the first buffer is handled, all the
        // callbacks should be sent at once.
        frame_callbacks.insert(end(frame_callbacks), begin(state.frame_callbacks), end(state.frame_callbacks));

        mw::Weak<mw::Buffer> const& weak_buffer = state.buffer.value();

        if (!weak_buffer)
        {
            // TODO: unmap surface, and unmap all subsurfaces
            buffer_size_ = std::nullopt;
            send_frame_callbacks(frame_callbacks);
        }
        else
        {
            std::function<void()> release_buffer;

            if (state.release_fence)
            {
                release_buffer = [release = *state.release_fence]() mutable
                    {
                        release.signal();
                    };
            }
            else
            {
                release_buffer = [executor = wayland_executor, weak_buffer]()
                {
                    executor->spawn([weak_buffer]()
                        {
                            if (weak_buffer)
                            {
                                weak_buffer.value().send_release_event();
                            }
                        });
                };
            }

            auto executor_send_frame_callbacks = [executor = wayland_executor, weak_self = mw::make_weak(this)]()
                {
                    executor->spawn([weak_self]()
                        {
                            if (weak_self)
                            {
                                auto& self = weak_self.value();
                                self.send_frame_callbacks(self.frame_callbacks);
                            }
                        });
                };

            if (auto const shm_buffer = mw::Buffer::from<ShmBuffer>(weak_buffer))
            {
                current_buffer = allocator->buffer_from_shm(
                    shm_buffer->data(),
                    std::move(executor_send_frame_callbacks),
                    std::move(release_buffer));
                tracepoint(
                    mir_server_wayland,
                    sw_buffer_committed,
                    client.get(),
                    current_buffer->id().as_value());
            }
            else if (auto const dmabuf = mw::Buffer::from<LinuxDmaBufBuffer>(weak_buffer))
            {
                current_buffer = dmabuf->import(
                    std::move(executor_send_frame_callbacks),
                    std::move(release_buffer));
                tracepoint(
                    mir_server_wayland,
                    hw_buffer_committed,
                    client.get(),
                    current_buffer->id().as_value());
            }
            else
            {
                mir::log_warning(
                    "wl_surface@%u committed an unrecognised buffer type", object_id());
                release_buffer();
                send_frame_callbacks(frame_callbacks);
            }

            needs_buffer_submission = true;
        }
    }
    else
    {
        if (state.release_fence)
        {
            throw mw::ProtocolError{
                sync_timeline.value().object_id(),
                mw::LinuxDrmSyncobjSurfaceV1::Error::no_buffer,
                "Timeline release sync point set, but no buffer committed"};
        }

        /*
         * Frame request committed with no associated buffer.
         * The Wayland protocol says that
         *
         * > When a client is animating on a wl_surface, it can use the 'frame'
         * > request to get notified when it is a good time to draw and commit
         * > the next frame of animation.
         *
         * If there's no *current* frame, then “now” is a good time to draw and
         * commit the next frame.
         *
         * Unfortunately, Firefox uses frame events as a heartbeat of sorts
         * (see issue #1967), so we can't send these callbacks immediately,
         * nor can we wait until Firefox sends us a frame to trigger them.
         *
         * A 60Hz (ish) timer for these was implemented in PR #2006, but this
         * triggered sending *all* current frame events when the 16ms timer elapsed,
         * resulting us sending frame events at suboptimal times and (eventually) breaking
         * WLCS tests which rely on the frame events for synchronisation.
         *
         * Rather than sending *all* frame events that have become pending after
         * 16ms, capture the current set of requested frame events. Then, after
         * the delay, send all these quirk frame callbacks.
         */
        if (!state.frame_callbacks.empty())
        {
            heartbeat_quirk_frame_callbacks.insert(
                end(heartbeat_quirk_frame_callbacks),
                begin(state.frame_callbacks),
                end(state.frame_callbacks));

            frame_callback_executor->spawn(
                [executor = wayland_executor, weak_self = mw::make_weak(this)]
                {
                    executor->spawn(
                        [weak_self]()
                        {
                            if (weak_self)
                            {
                                auto& self = weak_self.value();
                                self.send_frame_callbacks(self.heartbeat_quirk_frame_callbacks);
                            }
                        });
                }
            );
        }

    }

    if (needs_buffer_submission && current_buffer)
    {
        geom::Size logical_size;
        geom::RectangleD src_sample;

        if (viewport)
        {
            std::tie(src_sample, logical_size) = viewport.value().resolve_viewport(scale, current_buffer->size());
        }
        else
        {
            src_sample = geom::RectangleD{{0, 0}, current_buffer->size()};
            logical_size = current_buffer->size() / scale;
        }

        stream->submit_buffer(current_buffer, logical_size, src_sample);

        if (std::make_optional(logical_size) != buffer_size_)
        {
            state.invalidate_surface_data(); // input shape needs to be recalculated for the new size
        }

        buffer_size_ = logical_size;
    }

    for (WlSubsurface* child: children)
    {
        child->parent_has_committed();
    }
}

void mf::WlSurface::commit()
{
    if (pending.offset && *pending.offset == offset_)
        pending.offset = std::nullopt;

    // The same input shape could be represented by the same rectangles in a different order, or even
    // different rectangles. We don't check for that, however, because it would only cause an unnecessary
    // update and not do any real harm. Checking for identical vectors should cover most cases.
    if (pending.input_shape && *pending.input_shape == input_shape)
        pending.input_shape = std::nullopt;

    auto const [acquire_syncpoint, release_syncpoint] = [&]() -> std::pair<std::optional<SyncPoint>, std::optional<SyncPoint>>
        {
            if (sync_timeline && sync_timeline.value().timeline_set())
            {
                auto [acquire, release] = sync_timeline.value().claim_timeline();
                return std::make_pair(acquire, release);
            }
            return std::make_pair(std::nullopt, std::nullopt);
        }();

    /* Any previously committed, but not yet ready, buffer is now superceded.
     *
     * The fd listener is one-shot and non-cancellable, so we cannot remove the
     * in-flight registration. Instead bump the generation (invalidating the stale
     * listener, which will no-op when it fires) and signal the release fence so
     * the client is free to reuse the superseded buffer.
     */
    if (awaiting_buffer)
    {
        if (awaiting_release)
            awaiting_release->signal();
        ++buffer_ready_generation;
        awaiting_buffer = false;
        awaiting_release.reset();
    }

    if (acquire_syncpoint && release_syncpoint)
    {
        /* If we have an acquire syncpoint then we need to wait for the buffer to be ready
         * before we actually commit.
         *
         * In future we might want to push this further into the pipeline, but for now,
         * waiting before doing any of the commit is sufficient.
         */

        if (!pending.buffer || !(*pending.buffer))
        {
            throw mw::ProtocolError{
                sync_timeline.value().object_id(),
                mw::LinuxDrmSyncobjSurfaceV1::Error::no_buffer,
                "Timeline acquire sync point set, but no buffer committed"};
        }

        awaiting_buffer = true;
        awaiting_release = *release_syncpoint;
        ++buffer_ready_generation;

        auto const eventfd = acquire_syncpoint->to_eventfd();
        auto const eventfd_raw = static_cast<int>(eventfd);

        server.register_fd_ready_listener(
            eventfd_raw,
            std::make_unique<BufferReadyNotifier>(
                mw::make_weak(this),
                buffer_ready_generation,
                eventfd,
                *release_syncpoint));
    }
    else
    {
        complete_commit(this);
    }
}

void mf::WlSurface::complete_commit(WlSurface* surf)
{
    // Apply pending subsurface z-order changes BEFORE role->commit()
    // Per Wayland spec, subsurface state changes (z-order, position) are applied
    // immediately on parent commit, even if the parent is a synchronized subsurface
    if (surf->pending_surface_order)
    {
        // Convert list back to vector
        surf->children.assign(surf->pending_surface_order->begin(), surf->pending_surface_order->end());

        // Update parent_z_index and clear the parent marker
        auto const parent_marker = std::find(surf->children.begin(), surf->children.end(), nullptr);
        surf->parent_z_index = parent_marker - surf->children.begin();
        surf->children.erase(parent_marker);

        surf->pending_surface_order = std::nullopt;
    }

    // order is important
    auto const state = std::move(surf->pending);
    surf->pending = WlSurfaceState();
    surf->role->commit(state);

    if (surf->scene_surface_created_callbacks.size())
    {
        if (auto const surface = surf->scene_surface(); surface && surface.value())
        {
            for (auto const& callback : surf->scene_surface_created_callbacks)
            {
                callback(surface.value());
            }
            surf->scene_surface_created_callbacks.clear();
        }
    }
}

void mf::WlSurface::set_buffer_transform(uint32_t transform)
{
    try
    {
        auto const result = OutputManager::from_output_transform(transform);
        pending.orientation = std::get<0>(result);
        pending.mirror_mode = std::get<1>(result);
    }
    catch (std::out_of_range const&)
    {
        throw mw::ProtocolError{
            object_id(),
            Error::invalid_transform,
            "Invalid transform"};
    }

}

void mf::WlSurface::set_buffer_scale(int32_t scale)
{
    if (scale <= 0)
    {
        throw mw::ProtocolError{
            object_id(),
            Error::invalid_scale,
            "Invalid scale %d", scale};
    }
    pending.scale = scale;
}

void mir::frontend::WlSurface::offset(int32_t x, int32_t y)
{
    if (x != 0 || y != 0)
    {
        mir::log_warning("Client requested unimplemented non-zero offset. Rendering will be incorrect.");
    }
}

auto mf::WlSurface::confine_pointer_state() const -> MirPointerConfinementState
{
    if (auto const maybe_scene_surface = scene_surface())
    {
        if (auto const scene_surface = *maybe_scene_surface)
        {
            return scene_surface->confine_pointer_state();
        }
    }

    return mir_pointer_unconfined;
}

void mf::WlSurface::associate_viewport(mw::Weak<Viewport> viewport)
{
    if (this->viewport || pending.viewport)
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"Cannot associate a viewport to a window with an existing viewport"}));
    }
    pending.viewport = viewport;
}

void mf::WlSurface::associate_sync_timeline(mw::Weak<SyncTimeline> timeline)
{
    if (this->sync_timeline)
    {
        BOOST_THROW_EXCEPTION(TimelineAlreadyAssociated{});
    }
    sync_timeline = timeline;
}

void mir::frontend::WlSurface::update_surface_spec(shell::SurfaceSpecification const& spec)
{
    pending.surface_spec.update_from(spec);
}

mf::NullWlSurfaceRole::NullWlSurfaceRole(WlSurface* surface) :
    surface{surface}
{
}

auto mf::NullWlSurfaceRole::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    return std::nullopt;
}

void mf::WlSurface::set_fractional_scale(mir::frontend::FractionalScaleV1* fractional_scale)
{
    this->fractional_scale = mw::make_weak(fractional_scale);
}

auto mf::WlSurface::get_fractional_scale() const -> mw::Weak<FractionalScaleV1>
{
    return fractional_scale;
}

void mf::NullWlSurfaceRole::refresh_surface_data_now() {}
void mf::NullWlSurfaceRole::commit(WlSurfaceState const& state) { surface->commit(state); }
void mf::NullWlSurfaceRole::surface_destroyed() {}
