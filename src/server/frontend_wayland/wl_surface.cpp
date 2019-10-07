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

#include "wl_surface.h"

#include "wayland_utils.h"
#include "wl_surface_role.h"
#include "wl_subcompositor.h"
#include "wl_region.h"
#include "wlshmbuffer.h"
#include "deleted_for_resource.h"

#include "wayland_wrapper.h"

#include "wayland_frontend.tp.h"

#include "mir/graphics/buffer_properties.h"
#include "mir/scene/session.h"
#include "mir/frontend/wayland.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/executor.h"
#include "mir/graphics/wayland_allocator.h"
#include "mir/shell/surface_specification.h"
#include "mir/log.h"

#include <algorithm>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland;
namespace msh = mir::shell;

mf::WlSurfaceState::Callback::Callback(wl_resource* new_resource)
    : mw::Callback{new_resource, Version<1>()},
      destroyed{deleted_flag_for_resource(resource)}
{
}

void mf::WlSurfaceState::update_from(WlSurfaceState const& source)
{
    if (source.buffer)
        buffer = source.buffer;

    if (source.offset)
        offset = source.offset;

    if (source.input_shape)
        input_shape = source.input_shape;

    frame_callbacks.insert(end(frame_callbacks),
                           begin(source.frame_callbacks),
                           end(source.frame_callbacks));

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
    std::shared_ptr<Executor> const& executor,
    std::shared_ptr<graphics::WaylandAllocator> const& allocator)
    : Surface(new_resource, Version<4>()),
        session{get_session(client)},
        stream{session->create_buffer_stream({{}, mir_pixel_format_invalid, graphics::BufferUsage::undefined})},
        allocator{allocator},
        executor{executor},
        null_role{this},
        role{&null_role},
        destroyed{std::make_shared<bool>(false)}
{
    // wl_surface is specified to act in mailbox mode
    stream->allow_framedropping(true);
}

mf::WlSurface::~WlSurface()
{
    *destroyed = true;

    // so that unregister_destroy_listener calls invoked from destroy listeners don't screw up the iterator
    auto listeners = move(destroy_listeners);
    destroy_listeners.clear();
    for (auto listener: listeners)
    {
        listener.second();
    }

    role->destroy();
    session->destroy_buffer_stream(stream);
}

bool mf::WlSurface::synchronized() const
{
    return role->synchronized();
}

mf::WlSurface::Position mf::WlSurface::transform_point(geom::Point point)
{
    point = point - offset_;
    // loop backwards so the first subsurface we find that accepts the input is the topmost one
    for (auto child_it = children.rbegin(); child_it != children.rend(); ++child_it)
    {
        auto result = (*child_it)->transform_point(point);
        if (result.is_in_input_region)
            return result;
    }
    geom::Rectangle surface_rect = {geom::Point{}, buffer_size_.value_or(geom::Size{})};
    for (auto& rect : input_shape.value_or(std::vector<geom::Rectangle>{{{}, buffer_size_.value_or(geom::Size{})}}))
    {
        if (rect.intersection_with(surface_rect).contains(point))
            return {point, this, true};
    }
    return {point, this, false};
}

auto mf::WlSurface::scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>>
{
    return role->scene_surface();
}

void mf::WlSurface::set_role(WlSurfaceRole* role_)
{
    if (role != &null_role)
        BOOST_THROW_EXCEPTION(std::logic_error("Surface already has a role"));
    role = role_;
}

void mf::WlSurface::clear_role()
{
    role = &null_role;
}

void mf::WlSurface::set_pending_offset(std::experimental::optional<geom::Displacement> const& offset)
{
    pending.offset = offset;
}

std::unique_ptr<mf::WlSurface, std::function<void(mf::WlSurface*)>> mf::WlSurface::add_child(WlSubsurface* child)
{
    children.push_back(child);

    return std::unique_ptr<WlSurface, std::function<void(WlSurface*)>>(
        this,
        [child=child, destroyed=destroyed](WlSurface* self)
        {
            if (*destroyed)
                return;
            // remove the child from the vector
            self->children.erase(std::remove(self->children.begin(),
                                             self->children.end(),
                                             child),
                                 self->children.end());
        });
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

    buffer_streams.push_back(msh::StreamSpecification{stream, offset, {}});
    geom::Rectangle surface_rect = {geom::Point{} + offset, buffer_size_.value_or(geom::Size{})};
    if (input_shape)
    {
        for (auto rect : input_shape.value())
        {
            rect.top_left = rect.top_left + offset;
            rect = rect.intersection_with(surface_rect); // clip to surface
            input_shape_accumulator.push_back(rect);
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

void mf::WlSurface::add_destroy_listener(void const* key, std::function<void()> listener)
{
    destroy_listeners[key] = listener;
}

void mf::WlSurface::remove_destroy_listener(void const* key)
{
    destroy_listeners.erase(key);
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
        if (!*frame->destroyed)
        {
            // TODO: argument should be a timestamp
            frame->send_done_event(0);
            frame->destroy_wayland_object();
        }
    }
    frame_callbacks.clear();
}

void mf::WlSurface::destroy()
{
    *destroyed = true;
    destroy_wayland_object();
}

void mf::WlSurface::attach(std::experimental::optional<wl_resource*> const& buffer, int32_t x, int32_t y)
{
    if (x != 0 || y != 0)
    {
        mir::log_warning("Client requested unimplemented non-zero attach offset. Rendering will be incorrect.");
    }

    role->visiblity(!!buffer);

    pending.buffer = buffer.value_or(nullptr);
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
    pending.frame_callbacks.push_back(std::make_shared<WlSurfaceState::Callback>(new_callback));
}

void mf::WlSurface::set_opaque_region(std::experimental::optional<wl_resource*> const& region)
{
    (void)region;
    // This isn't essential, but could enable optimizations
}

void mf::WlSurface::set_input_region(std::experimental::optional<wl_resource*> const& region)
{
    if (region)
    {
        // since pending.input_shape is an optional optional, this is needed
        auto shape = WlRegion::from(region.value())->rectangle_vector();
        pending.input_shape = decltype(pending.input_shape)::value_type{move(shape)};
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

    if (state.buffer)
    {
        wl_resource * buffer = *state.buffer;

        if (buffer == nullptr)
        {
            // TODO: unmap surface, and unmap all subsurfaces
            buffer_size_ = std::experimental::nullopt;
            send_frame_callbacks();
        }
        else
        {
            auto const executor_send_frame_callbacks = [this, executor = executor, destroyed = destroyed]()
                {
                    executor->spawn(run_unless(
                        destroyed,
                        [this]()
                        {
                            send_frame_callbacks();
                        }));
                };

            std::shared_ptr<graphics::Buffer> mir_buffer;

            if (wl_shm_buffer_get(buffer))
            {
                mir_buffer = WlShmBuffer::mir_buffer_from_wl_buffer(
                    buffer,
                    executor,
                    std::move(executor_send_frame_callbacks));
                tracepoint(
                    mir_server_wayland,
                    sw_buffer_committed,
                    wl_resource_get_client(resource),
                    mir_buffer->id().as_value());
            }
            else
            {
                std::shared_ptr<bool> buffer_destroyed = deleted_flag_for_resource(buffer);

                auto release_buffer = [executor = executor, buffer = buffer, destroyed = buffer_destroyed]()
                    {
                        executor->spawn(run_unless(
                            destroyed,
                            [buffer](){ wl_resource_post_event(buffer, wayland::Buffer::Opcode::release); }));
                    };

                mir_buffer = allocator->buffer_from_resource(
                    buffer,
                    std::move(executor_send_frame_callbacks),
                    std::move(release_buffer));
                tracepoint(
                    mir_server_wayland,
                    hw_buffer_committed,
                    wl_resource_get_client(resource),
                    mir_buffer->id().as_value());
            }

            if (!input_shape && (!buffer_size_ || mir_buffer->size() != buffer_size_.value()))
            {
                state.invalidate_surface_data(); // input shape needs to be recalculated for the new size
            }
            buffer_size_ = mir_buffer->size();
            stream->submit_buffer(mir_buffer);
        }
    }
    else
    {
        send_frame_callbacks();
    }

    for (WlSubsurface* child: children)
    {
        child->parent_has_committed();
    }
}

void mf::WlSurface::commit()
{
    // order is important
    auto const state = std::move(pending);
    pending = WlSurfaceState();
    role->commit(state);
}

void mf::WlSurface::set_buffer_transform(int32_t transform)
{
    (void)transform;
    // TODO
}

void mf::WlSurface::set_buffer_scale(int32_t scale)
{
    (void)scale;
    // TODO
}

mf::NullWlSurfaceRole::NullWlSurfaceRole(WlSurface* surface) :
    surface{surface}
{
}

auto mf::NullWlSurfaceRole::scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>>
{
    return std::experimental::nullopt;
}
void mf::NullWlSurfaceRole::refresh_surface_data_now() {}
void mf::NullWlSurfaceRole::commit(WlSurfaceState const& state) { surface->commit(state); }
void mf::NullWlSurfaceRole::visiblity(bool /*visible*/) {}
void mf::NullWlSurfaceRole::destroy() {}
