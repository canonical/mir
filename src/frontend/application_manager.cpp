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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/frontend/application_manager.h"

#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_controller.h"

#include <memory>
#include <cassert>

namespace mf = mir::frontend;
namespace ms = mir::surfaces;

mf::ApplicationManager::ApplicationManager(ms::ApplicationSurfaceOrganiser* organiser) : surface_organiser(organiser)
{
    assert(surface_organiser);
}

std::weak_ptr<ms::Surface> mf::ApplicationManager::create_surface(const ms::SurfaceCreationParameters& params)
{
    return surface_organiser->create_surface(params);
}

void mf::ApplicationManager::destroy_surface(std::weak_ptr<ms::Surface> surface)
{
    surface_organiser->destroy_surface(surface);
}
