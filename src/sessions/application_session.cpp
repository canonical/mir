/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/sessions/application_session.h"
#include "mir/sessions/surface.h"

#include "mir/surfaces/surface_controller.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <memory>
#include <cassert>
#include <algorithm>

namespace msess = mir::sessions;
namespace ms = mir::surfaces;

msess::ApplicationSession::ApplicationSession(
    std::shared_ptr<msess::SurfaceFactory> const& surface_factory,
    std::string const& session_name) :
    surface_factory(surface_factory),
    session_name(session_name),
    next_surface_id(0)
{
    assert(surface_factory);
}

msess::ApplicationSession::~ApplicationSession()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto const& pair_id_surface : surfaces)
    {
        pair_id_surface.second->destroy();
    }
}

msess::SurfaceId msess::ApplicationSession::next_id()
{
    return SurfaceId(next_surface_id.fetch_add(1));
}

msess::SurfaceId msess::ApplicationSession::create_surface(const SurfaceCreationParameters& params)
{
    auto surf = surface_factory->create_surface(params);
    auto const id = next_id();

    std::unique_lock<std::mutex> lock(surfaces_mutex);
    surfaces[id] = surf;
    return id;
}

msess::ApplicationSession::Surfaces::const_iterator msess::ApplicationSession::checked_find(SurfaceId id) const
{
    auto p = surfaces.find(id);
    if (p == surfaces.end())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid SurfaceId"));
    }
    return p;
}

std::shared_ptr<msess::Surface> msess::ApplicationSession::get_surface(msess::SurfaceId id) const
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);

    return checked_find(id)->second;
}

void msess::ApplicationSession::destroy_surface(msess::SurfaceId id)
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    auto p = checked_find(id);

    p->second->destroy();
    surfaces.erase(p);
}

std::string msess::ApplicationSession::name()
{
    return session_name;
}

void msess::ApplicationSession::shutdown()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->shutdown();
    }
}

void msess::ApplicationSession::hide()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->hide();
    }
}

void msess::ApplicationSession::show()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->show();
    }
}
