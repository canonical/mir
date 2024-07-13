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

#include "viewporter_wrapper.h"
#include "wayland_utils.h"
#include "wl_surface_role.h"
#include "wl_subcompositor.h"
#include "wl_region.h"
#include "shm.h"
#include "resource_lifetime_tracker.h"

#include "wayland_wrapper.h"

#include "wayland_frontend.tp.h"

#include "mir/wayland/protocol_error.h"
#include "mir/wayland/client.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/scene/session.h"
#include "mir/frontend/wayland.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/executor.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/scene/surface.h"
#include "mir/shell/surface_specification.h"
#include "mir/log.h"
#include "wp_viewporter.h"

#include <chrono>
#include <boost/throw_exception.hpp>
#include <cmath>
#include <limits>
#include <wayland-server-protocol.h>

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland;
namespace msh = mir::shell;

mf::WlSurfaceState::Callback::Callback(wl_resource* new_resource)
    : mw::Callback{new_resource, Version<1>()}
{
}

void mf::WlSurfaceState::update_from(WlSurfaceState const& source)
{
    if (source.buffer)
        buffer = source.buffer;

    if (source.scale)
        scale = source.scale;

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
}

bool mf::WlSurfaceState::surface_data_needs_refresh() const
{
    return offset ||
           input_shape ||
           surface_data_invalidated;
}

mf::WlSurface::WlSurface(
    wl_resource* new_resource,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<Executor> const& frame_callback_executor,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator)
    : Surface(new_resource, Version<4>()),
        session{client->client_session()},
        stream{session->create_buffer_stream({{}, mir_pixel_format_invalid, graphics::BufferUsage::undefined})},
        allocator{allocator},
        wayland_executor{wayland_executor},
        frame_callback_executor{frame_callback_executor},
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
        mw::internal_error_processing_request(client->raw_client(), "WlSurface::~WlSurface()");
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
    for (auto child_it = children.rbegin(); child_it != children.rend(); ++child_it)
    {
        if (auto result = (*child_it)->subsurface_at(point))
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
    children.erase(
        std::remove(
            children.begin(),
            children.end(),
            child),
        children.end());
}

void mf::WlSurface::refresh_surface_data_now()
{
    role->refresh_surface_data_now();
}

void mf::WlSurface::populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                                          std::vector<geom::Rectangle>& input_shape_accumulator,
                                          geometry::Displacement const& parent_offset) const
{
    geometry::Displacement offset = parent_offset + offset_;

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

    for (WlSubsurface* subsurface : children)
    {
        subsurface->populate_surface_data(buffer_streams, input_shape_accumulator, offset);
    }
}

mf::WlSurface* mf::WlSurface::from(wl_resource* resource)
{
    void* raw_surface = wl_resource_get_user_data(resource);
    return static_cast<WlSurface*>(static_cast<wayland::Surface*>(raw_surface));
}

void mf::WlSurface::send_frame_callbacks()
{
    for (auto const& frame : frame_callbacks)
    {
        if (frame)
        {
            auto const timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch());
            frame.value().send_done_event(timestamp_ms.count());
            frame.value().destroy_and_delete();
        }
    }
    frame_callbacks.clear();
}

void mf::WlSurface::attach(std::optional<wl_resource*> const& buffer, int32_t x, int32_t y)
{
    if (x != 0 || y != 0)
    {
        mir::log_warning("Client requested unimplemented non-zero attach offset. Rendering will be incorrect.");
    }

    pending.buffer = mw::make_weak(buffer ? ResourceLifetimeTracker::from(buffer.value()) : nullptr);
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

void mf::WlSurface::frame(wl_resource* new_callback)
{
    auto callback = new WlSurfaceState::Callback{new_callback};
    pending.frame_callbacks.push_back(wayland::make_weak(callback));
}

void mf::WlSurface::set_opaque_region(std::optional<wl_resource*> const& region)
{
    (void)region;
    // This isn't essential, but could enable optimizations
}

void mf::WlSurface::set_input_region(std::optional<wl_resource*> const& region)
{
    if (region)
    {
        // since pending.input_shape is an optional optional, this is needed
        auto shape = WlRegion::from(region.value())->rectangle_vector();
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
    // We're going to lose the value of state, so copy the frame_callbacks first. We have to maintain a list of
    // callbacks in wl_surface because if a client commits multiple times before the first buffer is handled, all the
    // callbacks should be sent at once.
    frame_callbacks.insert(end(frame_callbacks), begin(state.frame_callbacks), end(state.frame_callbacks));

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
        (viewport && viewport.value().changed_since_last_resolve()); // ...the viewport has changed...
                                                                     // ...then we'll need to submit a new frame, even if the client hasn't
                                                                     // attached a new buffer.

    auto const executor_send_frame_callbacks = [executor = wayland_executor, weak_self = mw::make_weak(this)]()
        {
            executor->spawn([weak_self]()
                {
                    if (weak_self)
                    {
                        weak_self.value().send_frame_callbacks();
                    }
                });
        };

    if (state.buffer)
    {
        mw::Weak<ResourceLifetimeTracker> const& weak_buffer = state.buffer.value();

        if (!weak_buffer)
        {
            // TODO: unmap surface, and unmap all subsurfaces
            buffer_size_ = std::nullopt;
            send_frame_callbacks();
        }
        else
        {
            auto release_buffer = [executor = wayland_executor, weak_buffer]()
                {
                    executor->spawn([weak_buffer]()
                        {
                            if (weak_buffer)
                            {
                                wl_resource_post_event(weak_buffer.value(), wayland::Buffer::Opcode::release);
                            }
                        });
                };

            if (auto const shm_buffer = ShmBuffer::from(weak_buffer.value()))
            {
                current_buffer = allocator->buffer_from_shm(
                    shm_buffer->data(),
                    std::move(executor_send_frame_callbacks),
                    std::move(release_buffer));
                tracepoint(
                    mir_server_wayland,
                    sw_buffer_committed,
                    wl_resource_get_client(resource),
                    current_buffer->id().as_value());
            }
            else
            {
                current_buffer = allocator->buffer_from_resource(
                    weak_buffer.value(),
                    std::move(executor_send_frame_callbacks),
                    std::move(release_buffer));
                tracepoint(
                    mir_server_wayland,
                    hw_buffer_committed,
                    wl_resource_get_client(resource),
                    current_buffer->id().as_value());
            }

            needs_buffer_submission = true;
        }
    }
    else
    {
        frame_callback_executor->spawn(std::move(executor_send_frame_callbacks));
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

    // order is important
    auto const state = std::move(pending);
    pending = WlSurfaceState();
    role->commit(state);

    if (scene_surface_created_callbacks.size())
    {
        if (auto const surface = scene_surface(); surface && surface.value())
        {
            for (auto const& callback : scene_surface_created_callbacks)
            {
                callback(surface.value());
            }
            scene_surface_created_callbacks.clear();
        }
    }
}

void mf::WlSurface::set_buffer_transform(int32_t transform)
{
    (void)transform;
    // TODO
}

void mf::WlSurface::set_buffer_scale(int32_t scale)
{
    pending.scale = scale;
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

void mf::WlSurface::associate_viewport(wayland::Weak<Viewport> viewport)
{
    if (this->viewport || pending.viewport)
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"Cannot associate a viewport to a window with an existing viewport"}));
    }
    pending.viewport = viewport;
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
void mf::NullWlSurfaceRole::refresh_surface_data_now() {}
void mf::NullWlSurfaceRole::commit(WlSurfaceState const& state) { surface->commit(state); }
void mf::NullWlSurfaceRole::surface_destroyed() {}
