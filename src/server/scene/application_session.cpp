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
#include "trust_session.h"
#include "mir/shell/surface.h"
#include "mir/shell/surface_factory.h"
#include "snapshot_strategy.h"
#include "mir/shell/session_listener.h"
#include "mir/frontend/event_sink.h"
#include "default_session_container.h"
#include "mir/shell/trust_session.h"

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

ms::ApplicationSession::ApplicationSession(
    std::shared_ptr<msh::SurfaceFactory> const& surface_factory,
    pid_t pid,
    std::string const& session_name,
    std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
    std::shared_ptr<msh::SessionListener> const& session_listener,
    std::shared_ptr<mf::EventSink> const& sink) :
    surface_factory(surface_factory),
    pid(pid),
    session_name(session_name),
    snapshot_strategy(snapshot_strategy),
    session_listener(session_listener),
    event_sink(sink),
    next_surface_id(0)
{
    assert(surface_factory);
}

ms::ApplicationSession::~ApplicationSession()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto const& pair_id_surface : surfaces)
    {
        session_listener->destroying_surface(*this, pair_id_surface.second);
    }

    clear_trusted_children();
}

mf::SurfaceId ms::ApplicationSession::next_id()
{
    return mf::SurfaceId(next_surface_id.fetch_add(1));
}

mf::SurfaceId ms::ApplicationSession::create_surface(const msh::SurfaceCreationParameters& params)
{
    auto const id = next_id();
    auto surf = surface_factory->create_surface(this, params, id, event_sink);

    std::unique_lock<std::mutex> lock(surfaces_mutex);
    surfaces[id] = surf;

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
    std::unique_lock<std::mutex> lock(surfaces_mutex);

    return checked_find(id)->second;
}

void ms::ApplicationSession::take_snapshot(msh::SnapshotCallback const& snapshot_taken)
{
    if (auto surface = default_surface())
        snapshot_strategy->take_snapshot_of(surface, snapshot_taken);
    else
        snapshot_taken(msh::Snapshot());
}

std::shared_ptr<msh::Surface> ms::ApplicationSession::default_surface() const
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);

    if (surfaces.size())
        return surfaces.begin()->second;
    else
        return std::shared_ptr<msh::Surface>();
}

void ms::ApplicationSession::destroy_surface(mf::SurfaceId id)
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    auto p = checked_find(id);

    session_listener->destroying_surface(*this, p->second);

    surfaces.erase(p);
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
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->force_requests_to_complete();
    }
}

void ms::ApplicationSession::hide()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->hide();
    }

    for_each_trusted_child(
        [](std::shared_ptr<shell::Session> const& child_session)
        {
            child_session->hide();
            return true;
        },
        false
    );
}

void ms::ApplicationSession::show()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->show();
    }

    for_each_trusted_child(
        [](std::shared_ptr<shell::Session> const& child_session)
        {
            child_session->show();
            return true;
        },
        false
    );
}

int ms::ApplicationSession::configure_surface(mf::SurfaceId id,
                                               MirSurfaceAttrib attrib,
                                               int requested_value)
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    std::shared_ptr<msh::Surface> surf(checked_find(id)->second);

    return surf->configure(attrib, requested_value);
}

void ms::ApplicationSession::send_display_config(mg::DisplayConfiguration const& info)
{
    event_sink->handle_display_config_change(info);
}

void ms::ApplicationSession::set_lifecycle_state(MirLifecycleState state)
{
    event_sink->handle_lifecycle_event(state);


    for_each_trusted_child(
        [state](std::shared_ptr<shell::Session> const& child_session)
        {
            child_session->set_lifecycle_state(state);
            return true;
        },
        false
    );
}

void ms::ApplicationSession::begin_trust_session(std::shared_ptr<msh::TrustSession> const& trust_session,
                                                 std::vector<std::shared_ptr<msh::Session>> const& trusted_children)
{
    set_trust_session(trust_session);
    for(auto const& trusted_child : trusted_children)
    {
        add_trusted_child(trusted_child);
    }

    // All sessions which are part of the trust session get this event.
    MirEvent start_event;
    memset(&start_event, 0, sizeof start_event);
    start_event.type = mir_event_type_trust_session_state_change;
    start_event.trust_session.new_state = mir_trust_session_state_started;
    event_sink->handle_event(start_event);

    session_listener->trust_session_started(*this, trust_session);
}

void ms::ApplicationSession::end_trust_session()
{
    session_listener->trust_session_stopping(*this, get_trust_session());

    for_each_trusted_child([](std::shared_ptr<msh::Session> const& child_session)
        {
            child_session->end_trust_session();
            return true;
        },
        false
    );
    clear_trusted_children();
    set_trust_session(nullptr);

    MirEvent stop_event;
    memset(&stop_event, 0, sizeof stop_event);
    stop_event.type = mir_event_type_trust_session_state_change;
    stop_event.trust_session.new_state = mir_trust_session_state_stopped;
    event_sink->handle_event(stop_event);
}

void ms::ApplicationSession::add_trusted_child(std::shared_ptr<msh::Session> const& session)
{
    {
        std::unique_lock<std::mutex> lock(mutex_trusted_children);
        trusted_children.push_back(session);
    }

    session->begin_trust_session(get_trust_session(), std::vector<std::shared_ptr<Session>>());
}

void ms::ApplicationSession::remove_trusted_child(std::shared_ptr<msh::Session> const& session)
{
    std::unique_lock<std::mutex> lock(mutex_trusted_children);

    for (auto it = trusted_children.begin(); it != trusted_children.end(); ++it) {
        if (*it == session) {
            trusted_children.erase(it);
            break;
        }
    }
}

void ms::ApplicationSession::for_each_trusted_child(
    std::function<bool(std::shared_ptr<shell::Session> const&)> f,
    bool reverse) const
{
    std::unique_lock<std::mutex> lk(mutex_trusted_children);

    if (reverse)
    {
        for (auto rit = trusted_children.rbegin(); rit != trusted_children.rend(); ++rit)
        {
            if (!f(*rit))
                break;
        }
    }
    else
    {
        for (auto it = trusted_children.begin(); it != trusted_children.end(); ++it)
        {
            if (!f(*it))
                break;
        }
    }
}

std::shared_ptr<msh::TrustSession> ms::ApplicationSession::get_trust_session() const
{
    std::unique_lock<std::mutex> lock(mutex_trusted_session);

    return trust_session;
}

void ms::ApplicationSession::set_trust_session(std::shared_ptr<msh::TrustSession> const& _trust_session)
{
    std::unique_lock<std::mutex> lock(mutex_trusted_session);

    trust_session = _trust_session;
}

void ms::ApplicationSession::clear_trusted_children()
{
    std::unique_lock<std::mutex> lock(mutex_trusted_children);
    trusted_children.clear();
}