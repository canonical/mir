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

#include "mir/sessions/session.h"
#include "mir/sessions/surface.h"

#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_controller.h"

#include <memory>
#include <cassert>
#include <algorithm>
#include <stdio.h>

namespace msess = mir::sessions;
namespace ms = mir::surfaces;

msess::Session::Session(
    std::shared_ptr<msess::SurfaceFactory> const& organiser,
    std::string const& session_name) :
    surface_organiser(organiser),
    session_name(session_name),
    next_surface_id(0)
{
    assert(surface_organiser);
}

msess::Session::~Session()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto const& pair_id_surface : surfaces)
    {
        pair_id_surface.second->destroy();
    }
}

msess::SurfaceId msess::Session::next_id()
{
    return SurfaceId(next_surface_id.fetch_add(1));
}

msess::SurfaceId msess::Session::create_surface(const SurfaceCreationParameters& params)
{
    auto surf = surface_organiser->create_surface(params);
    auto const id = next_id();

    std::unique_lock<std::mutex> lock(surfaces_mutex);
    surfaces[id] = surf;
    return id;
}

std::shared_ptr<msess::Surface> msess::Session::get_surface(msess::SurfaceId id) const
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    return surfaces.find(id)->second;
}

void msess::Session::destroy_surface(msess::SurfaceId id)
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    auto p = surfaces.find(id);

    if (p != surfaces.end())
    {
        p->second->destroy();
        surfaces.erase(p);
    }
}

std::string msess::Session::name()
{
    return session_name;
}

void msess::Session::shutdown()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->shutdown();
    }
}

void msess::Session::hide()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->hide();
    }
}

void msess::Session::show()
{
    std::unique_lock<std::mutex> lock(surfaces_mutex);
    for (auto& id_s : surfaces)
    {
        id_s.second->show();
    }
}
