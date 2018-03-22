/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Robert Carr <racarr@canonical.com>
 */

#include "application_session.h"
#include "snapshot_strategy.h"
#include "default_session_container.h"
#include "output_properties_cache.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_event_source.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/session_listener.h"
#include "mir/scene/surface_factory.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/shell/surface_stack.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/events/event_builders.h"
#include "mir/frontend/event_sink.h"
#include "mir/graphics/graphic_buffer_allocator.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <memory>
#include <cassert>
#include <algorithm>
#include <cstring>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mev = mir::events;
namespace mc = mir::compositor;

ms::ApplicationSession::ApplicationSession(
    std::shared_ptr<msh::SurfaceStack> const& surface_stack,
    std::shared_ptr<SurfaceFactory> const& surface_factory,
    std::shared_ptr<ms::BufferStreamFactory> const& buffer_stream_factory,
    pid_t pid,
    std::string const& session_name,
    std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
    std::shared_ptr<SessionListener> const& session_listener,
    mg::DisplayConfiguration const& initial_config,
    std::shared_ptr<mf::EventSink> const& sink,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& gralloc) : 
    surface_stack(surface_stack),
    surface_factory(surface_factory),
    buffer_stream_factory(buffer_stream_factory),
    pid(pid),
    session_name(session_name),
    snapshot_strategy(snapshot_strategy),
    session_listener(session_listener),
    event_sink(sink),
    gralloc(gralloc),
    next_surface_id(0)
{
    assert(surface_stack);
    output_cache.update_from(initial_config);
}

ms::ApplicationSession::~ApplicationSession()
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    for (auto const& pair_id_surface : surfaces)
    {
        session_listener->destroying_surface(*this, pair_id_surface.second);
        surface_stack->remove_surface(pair_id_surface.second);
    }
}

mf::SurfaceId ms::ApplicationSession::next_id()
{
    return mf::SurfaceId(next_surface_id.fetch_add(1));
}

mf::SurfaceId ms::ApplicationSession::create_surface(
    SurfaceCreationParameters const& the_params,
    std::shared_ptr<mf::EventSink> const& surface_sink)
{
    auto const id = next_id();

    //TODO: we take either the content_id or the first streams content for now.
    //      Once the surface factory interface takes more than one stream,
    //      we can take all the streams as content.
    if (!((the_params.content_id.is_set()) ||
          (the_params.streams.is_set() && the_params.streams.value().size() > 0)))
    {
        BOOST_THROW_EXCEPTION(std::logic_error("surface must have content"));
    }

    auto params = the_params;

    mf::BufferStreamId stream_id;
    std::shared_ptr<mc::BufferStream> buffer_stream;
    if (params.content_id.is_set())
    {
        stream_id = params.content_id.value();
        buffer_stream = checked_find(stream_id)->second;
        if (params.size != buffer_stream->stream_size())
            buffer_stream->resize(params.size);
    }
    else
    {
        stream_id = params.streams.value()[0].stream_id;
        buffer_stream = checked_find(stream_id)->second;
    }

    if (params.parent_id.is_set())
        params.parent = checked_find(the_params.parent_id.value())->second;

    std::list<StreamInfo> streams;
    if (the_params.content_id.is_set())
    {
        streams.push_back({checked_find(the_params.content_id.value())->second, {0,0}, {}});
    }
    else
    {
        for (auto& stream : params.streams.value())
            streams.push_back({checked_find(stream.stream_id)->second, stream.displacement, stream.size});
    }

    auto surface = surface_factory->create_surface(streams, params);

    surface_stack->add_surface(surface, params.input_mode);

    auto const observer = std::make_shared<scene::SurfaceEventSource>(
        id,
        *surface,
        output_cache,
        surface_sink);
    surface->add_observer(observer);

    {
        std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
        surfaces[id] = surface;
        default_content_map[id] = stream_id;
    }

    observer->moved_to(surface.get(), surface->top_left());

    session_listener->surface_created(*this, surface);

    if (params.state.is_set())
        surface->configure(mir_window_attrib_state, params.state.value());
    if (params.type.is_set())
        surface->configure(mir_window_attrib_type, params.type.value());
    if (params.preferred_orientation.is_set())
        surface->configure(mir_window_attrib_preferred_orientation, params.preferred_orientation.value());
    if (params.input_shape.is_set())
        surface->set_input_region(params.input_shape.value());

    return id;
}

ms::ApplicationSession::Surfaces::const_iterator ms::ApplicationSession::checked_find(mf::SurfaceId id) const
{
    auto p = surfaces.find(id);
    if (p == surfaces.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid SurfaceId"));
    return p;
}

ms::ApplicationSession::Streams::const_iterator ms::ApplicationSession::checked_find(mf::BufferStreamId id) const
{
    auto p = streams.find(id);
    if (p == streams.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid BufferStreamId"));
    return p;
}

std::shared_ptr<mf::Surface> ms::ApplicationSession::get_surface(mf::SurfaceId id) const
{
    return surface(id);
}

std::shared_ptr<ms::Surface> ms::ApplicationSession::surface(mf::SurfaceId id) const
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    return checked_find(id)->second;
}

std::shared_ptr<ms::Surface> ms::ApplicationSession::surface_after(std::shared_ptr<ms::Surface> const& before) const
{
    std::lock_guard<std::mutex> lock(surfaces_and_streams_mutex);
    auto current = surfaces.begin();
    for (; current != surfaces.end(); ++current)
    {
        if (current->second == before)
            break;
    }

    if (current == surfaces.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("surface_after: surface is not a member of this session"));

    auto const can_take_focus = [](Surfaces::value_type const &s)
        {
            switch (s.second->type())
            {
            case mir_window_type_normal:       /**< AKA "regular"                       */
            case mir_window_type_utility:      /**< AKA "floating"                      */
            case mir_window_type_dialog:
            case mir_window_type_satellite:    /**< AKA "toolbox"/"toolbar"             */
            case mir_window_type_freestyle:
            case mir_window_type_menu:
            case mir_window_type_inputmethod:  /**< AKA "OSK" or handwriting etc.       */
                return true;

            case mir_window_type_gloss:
            case mir_window_type_tip:          /**< AKA "tooltip"                       */
            default:
                // Cannot have input focus - skip it
                return false;
            }
        };

    auto next = std::find_if(++current, end(surfaces), can_take_focus);

    if (next == surfaces.end())
        next = std::find_if(begin(surfaces), current, can_take_focus);

    if (next == end(surfaces))
        return {};

    return next->second;
}

void ms::ApplicationSession::take_snapshot(SnapshotCallback const& snapshot_taken)
{
    //TODO: taking a snapshot of a session doesn't make much sense. Snapshots can be on surfaces
    //or bufferstreams, as those represent some content. A multi-surface session doesn't have enough
    //info to cobble together a snapshot buffer without WM info.
    for(auto const& surface_it : surfaces)
    {
        if (default_surface() == surface_it.second)
        {
            auto id = default_content_map[surface_it.first];
            snapshot_strategy->take_snapshot_of(checked_find(id)->second, snapshot_taken);
            return;
        }
    }

    snapshot_taken(Snapshot());
}

std::shared_ptr<ms::Surface> ms::ApplicationSession::default_surface() const
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);

    if (surfaces.size())
        return surfaces.begin()->second;
    else
        return std::shared_ptr<ms::Surface>();
}

void ms::ApplicationSession::destroy_surface(mf::SurfaceId id)
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);

    destroy_surface(lock, checked_find(id));
}

std::string ms::ApplicationSession::name() const
{
    return session_name;
}

pid_t ms::ApplicationSession::process_id() const
{
    return pid;
}

void ms::ApplicationSession::hide()
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->hide();
    }
}

void ms::ApplicationSession::show()
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->show();
    }
}

void ms::ApplicationSession::send_display_config(mg::DisplayConfiguration const& info)
{
    output_cache.update_from(info);

    event_sink->handle_display_config_change(info);

    std::lock_guard<std::mutex> lock{surfaces_and_streams_mutex};
    for (auto& surface : surfaces)
    {
        auto output_properties = output_cache.properties_for(geometry::Rectangle{
            surface.second->top_left(),
            surface.second->size()});

        if (output_properties)
        {
            event_sink->handle_event(
                *mev::make_event(
                    surface.first,
                    output_properties->dpi,
                    output_properties->scale,
                    output_properties->refresh_rate,
                    output_properties->form_factor,
                    static_cast<uint32_t>(output_properties->id.as_value())
                    ));
        }
    }
}

void ms::ApplicationSession::send_input_config(MirInputConfig const& config)
{
    event_sink->handle_input_config_change(config);
}

void ms::ApplicationSession::set_lifecycle_state(MirLifecycleState state)
{
    event_sink->handle_lifecycle_event(state);
}

void ms::ApplicationSession::start_prompt_session()
{
    // All sessions which are part of the prompt session get this event.
    event_sink->handle_event(*mev::make_event(mir_prompt_session_state_started));
}

void ms::ApplicationSession::stop_prompt_session()
{
    event_sink->handle_event(*mev::make_event(mir_prompt_session_state_stopped));
}

void ms::ApplicationSession::suspend_prompt_session()
{
    event_sink->handle_event(*mev::make_event(mir_prompt_session_state_suspended));
}

void ms::ApplicationSession::resume_prompt_session()
{
    start_prompt_session();
}

std::shared_ptr<mf::BufferStream> ms::ApplicationSession::get_buffer_stream(mf::BufferStreamId id) const
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    return checked_find(id)->second;
}

mf::BufferStreamId ms::ApplicationSession::create_buffer_stream(mg::BufferProperties const& props)
{
    auto const id = static_cast<mf::BufferStreamId>(next_id().as_value());
    auto stream = buffer_stream_factory->create_buffer_stream(id, props);
    session_listener->buffer_stream_created(*this, stream);

    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    streams[id] = stream;
    return id;
}

void ms::ApplicationSession::destroy_buffer_stream(mf::BufferStreamId id)
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    auto stream_it = streams.find(mir::frontend::BufferStreamId(id.as_value()));
    if (stream_it == streams.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot destroy stream: Invalid BufferStreamId"));

    session_listener->buffer_stream_destroyed(*this, stream_it->second);
    streams.erase(stream_it);
}

void ms::ApplicationSession::configure_streams(
    ms::Surface& surface, std::vector<shell::StreamSpecification> const& streams)
{
    std::list<ms::StreamInfo> list;
    for (auto& stream : streams)
    {
        auto s = checked_find(stream.stream_id)->second;
        list.emplace_back(ms::StreamInfo{s, stream.displacement, stream.size});
    }
    surface.set_streams(list); 
}

void ms::ApplicationSession::destroy_surface(std::weak_ptr<Surface> const& surface)
{
    auto const ss = surface.lock();
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    auto p = find_if(begin(surfaces), end(surfaces), [&](Surfaces::value_type const& val)
        { return val.second == ss; });

    if (p == surfaces.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid Surface"));

    destroy_surface(lock, p);
}

void ms::ApplicationSession::destroy_surface(std::unique_lock<std::mutex>& lock, Surfaces::const_iterator in_surfaces)
{
    auto const surface = in_surfaces->second;
    auto it = default_content_map.find(in_surfaces->first); 
    session_listener->destroying_surface(*this, surface);
    surfaces.erase(in_surfaces);

    if (it != default_content_map.end())
        default_content_map.erase(it);

    lock.unlock();

    surface_stack->remove_surface(surface);
}

void ms::ApplicationSession::send_error(mir::ClientVisibleError const& error)
{
    event_sink->handle_error(error);
}
