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

#include "mir/frontend/application_session.h"

#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_controller.h"

#include <memory>
#include <cassert>
#include <algorithm>
#include <stdio.h>

namespace mf = mir::frontend;
namespace ms = mir::surfaces;

mf::ApplicationSession::ApplicationSession(
    std::shared_ptr<ms::ApplicationSurfaceOrganiser> const& organiser,
    std::string const& application_name) :
    surface_organiser(organiser),
    name(application_name),
    next_surface_id(0)
{
    assert(surface_organiser);
}

mf::ApplicationSession::~ApplicationSession()
{
    for (auto it = surfaces.begin(); it != surfaces.end(); it++)
    {
        auto surface = it->second;
        surface_organiser->destroy_surface(surface);
    }
}

int mf::ApplicationSession::next_id()
{
    int id = next_surface_id.load();
    while (!next_surface_id.compare_exchange_weak(id, id + 1)) std::this_thread::yield();
    return id;
}

int mf::ApplicationSession::create_surface(const ms::SurfaceCreationParameters& params)
{
    auto surf = surface_organiser->create_surface(params);
    auto const id = next_id();

    surfaces[id] = surf;
    return id;
}

std::shared_ptr<ms::Surface> mf::ApplicationSession::get_surface(int id) const
{
    return surfaces.find(id)->second.lock();
}

void mf::ApplicationSession::destroy_surface(int id)
{
    auto p = surfaces.find(id);

    if (p != surfaces.end())
    {
        surface_organiser->destroy_surface(p->second);
        surfaces.erase(p);
    }
}

std::string mf::ApplicationSession::get_name()
{
    return name;
}

void mf::ApplicationSession::hide()
{
    for (auto it = surfaces.begin(); it != surfaces.end(); it++)
    {
        auto& surface = it->second;
        surface_organiser->hide_surface(surface);
    }
}

void mf::ApplicationSession::show()
{
    for (auto it = surfaces.begin(); it != surfaces.end(); it++)
    {
        auto& surface = it->second;
        surface_organiser->show_surface(surface);
    }
}
