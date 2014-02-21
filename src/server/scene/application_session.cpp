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
#include "trusted_session_impl.h"
#include "mir/shell/surface.h"
#include "mir/shell/surface_factory.h"
#include "snapshot_strategy.h"
#include "mir/shell/session_listener.h"
#include "mir/frontend/event_sink.h"
#include "default_session_container.h"
#include "mir/shell/trusted_session.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <memory>
#include <cassert>
#include <algorithm>

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
    next_surface_id(0),
    children(std::make_shared<DefaultSessionContainer>())
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

    printf("DESTROY session %s\n", session_name.c_str());
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
    snapshot_strategy->take_snapshot_of(default_surface(), snapshot_taken);
}

std::shared_ptr<msh::Surface> ms::ApplicationSession::default_surface() const
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);

    std::shared_ptr<msh::Surface> child_surface;
    children->for_each(
        [&child_surface]
        (std::shared_ptr<shell::Session> const& child_session)
        {
            child_surface = child_session->default_surface();
            if (child_surface)
            {
                printf("returning child surface from %s\n", child_session->name().c_str());
                return false;
            }
            printf("no surface for %s\n", child_session->name().c_str());
            return true;
        },
        true // reverse for_each
    );

    if (child_surface)
        return child_surface;

    if (surfaces.size())
    {
        printf("returning main surface from %s\n", name().c_str());
        return surfaces.begin()->second;
    }
    else
    {
        printf("no surface for %s\n", name().c_str());
        return std::shared_ptr<msh::Surface>();
    }
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
}

void ms::ApplicationSession::show()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->show();
    }
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
    children->for_each(
        [state]
        (std::shared_ptr<shell::Session> const& child_session)
        {
            child_session->set_lifecycle_state(state);
        }
    );
}

std::shared_ptr<msh::Session> ms::ApplicationSession::get_parent() const
{
    std::unique_lock<std::mutex> lock(mutex);

    return parent;
}

void ms::ApplicationSession::set_parent(std::shared_ptr<msh::Session> const& new_parent)
{
    std::unique_lock<std::mutex> lock(mutex);

    parent = new_parent;
}

std::shared_ptr<ms::SessionContainer> ms::ApplicationSession::get_children() const
{
    return children;
}

std::shared_ptr<msh::TrustedSession> ms::ApplicationSession::get_trusted_session() const
{
    std::unique_lock<std::mutex> lock(mutex);

    return trusted_session;
}

void ms::ApplicationSession::set_trusted_session(std::shared_ptr<msh::TrustedSession> const& _trusted_session)
{
    std::unique_lock<std::mutex> lock(mutex);

    trusted_session = _trusted_session;
}