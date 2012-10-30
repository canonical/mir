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

namespace mf = mir::frontend;
namespace ms = mir::surfaces;

mf::ApplicationSession::ApplicationSession(ms::ApplicationSurfaceOrganiser* organiser, std::string application_name) : surface_organiser(organiser),
                                                                                                                       name(application_name)
{
    next_surface_id = 0;
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

int mf::ApplicationSession::create_surface(const ms::SurfaceCreationParameters& params)
{
    auto surf = surface_organiser->create_surface(params);
    surfaces[next_surface_id] = surf;
    next_surface_id++;
    
    return (next_surface_id-1);
}

void mf::ApplicationSession::destroy_surface(int surface_id)
{
    auto surface = surfaces[surface_id];
    surface_organiser->destroy_surface(surface);

    // FIXME: What goes on with no surface ID?
    surfaces.erase(surface_id);
}

std::string mf::ApplicationSession::get_name()
{
    return name;
}

void mf::ApplicationSession::hide()
{
   for (auto it = surfaces.begin(); it != surfaces.end(); it++)
    {
        auto surface = it->second;
        surface.lock()->set_hidden(true);
    }
}

void mf::ApplicationSession::show()
{
   for (auto it = surfaces.begin(); it != surfaces.end(); it++)
    {
        auto surface = it->second;
        surface.lock()->set_hidden(false);
    }
}
