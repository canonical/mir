/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Robert Carr <racarr@canonical.com>
 */

#include "application_session.h"
#include "snapshot_strategy.h"
#include "default_session_container.h"
#include "surfaceless_buffer_stream.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_event_source.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/session_listener.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/events/event_builders.h"
#include "mir/frontend/event_sink.h"

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

ms::ApplicationSession::ApplicationSession(
    std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator,
    std::shared_ptr<ms::BufferStreamFactory> const& buffer_stream_factory,
    pid_t pid,
    std::string const& session_name,
    std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
    std::shared_ptr<SessionListener> const& session_listener,
    std::shared_ptr<mf::EventSink> const& sink) :
    surface_coordinator(surface_coordinator),
    buffer_stream_factory(buffer_stream_factory),
    pid(pid),
    session_name(session_name),
    snapshot_strategy(snapshot_strategy),
    session_listener(session_listener),
    event_sink(sink),
    next_surface_id(0)
{
    assert(surface_coordinator);
}

ms::ApplicationSession::~ApplicationSession()
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    for (auto const& pair_id_surface : surfaces)
    {
        session_listener->destroying_surface(*this, pair_id_surface.second);
        surface_coordinator->remove_surface(pair_id_surface.second);
    }
}

mf::SurfaceId ms::ApplicationSession::next_id()
{
    return mf::SurfaceId(next_surface_id.fetch_add(1));
}

mf::SurfaceId ms::ApplicationSession::create_surface(SurfaceCreationParameters const& the_params)
{
    auto const id = next_id();

    auto params = the_params;

    if (params.parent_id.is_set())
        params.parent = checked_find(the_params.parent_id.value())->second;

    auto const observer = std::make_shared<scene::SurfaceEventSource>(id, event_sink);
    auto surf = surface_coordinator->add_surface(params, this);

    if (params.state.is_set())
        surf->configure(mir_surface_attrib_state, params.state.value());
    if (params.type.is_set())
        surf->configure(mir_surface_attrib_type, params.type.value());
    if (params.preferred_orientation.is_set())
        surf->configure(mir_surface_attrib_preferred_orientation, params.preferred_orientation.value());

    surf->add_observer(observer);

    {
        std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
        surfaces[id] = surf;
    }

    session_listener->surface_created(*this, surf);
    return id;
}

ms::ApplicationSession::Surfaces::const_iterator ms::ApplicationSession::checked_find(mf::SurfaceId id) const
{
    auto p = surfaces.find(id);
    if (p == surfaces.end())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid SurfaceId"));
    }
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
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    auto i = surfaces.begin();
    for (; i != surfaces.end(); ++i)
    {
        if (i->second == before)
            break;
    }
    if (i == surfaces.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("surface_after: surface is not a member of this session"));

    ++i;
    if (i == surfaces.end())
        i = surfaces.begin();
    return i->second;
}

void ms::ApplicationSession::take_snapshot(SnapshotCallback const& snapshot_taken)
{
    if (auto surface = default_surface())
        snapshot_strategy->take_snapshot_of(surface, snapshot_taken);
    else
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
    auto p = checked_find(id);
    auto const surface = p->second;

    session_listener->destroying_surface(*this, surface);

    surfaces.erase(p);
    lock.unlock();

    surface_coordinator->remove_surface(surface);
}

std::string ms::ApplicationSession::name() const
{
    return session_name;
}

pid_t ms::ApplicationSession::process_id() const
{
    return pid;
}

void ms::ApplicationSession::force_requests_to_complete()
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->force_requests_to_complete();
    }
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
    event_sink->handle_display_config_change(info);
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

    auto p = streams.find(id);
    if (p == streams.end())
        return checked_find(mf::SurfaceId(id.as_value()))->second;
    else 
        return p->second;
}

mf::BufferStreamId ms::ApplicationSession::create_buffer_stream(mg::BufferProperties const& props)
{
    auto const id = static_cast<mf::BufferStreamId>(next_id().as_value());
    auto stream = std::make_shared<ms::SurfacelessBufferStream>(buffer_stream_factory->create_buffer_stream(2, props));
    
    {
        std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
        streams[id] = stream;
    }

    return id;
}

void ms::ApplicationSession::destroy_buffer_stream(mf::BufferStreamId id)
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
    auto p = streams.find(id);
    if (p == streams.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid buffer stream id"));
    auto const stream = p->second;

    streams.erase(p);
}
