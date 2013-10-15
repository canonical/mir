/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "surfaces_report.h"

#include "mir/surfaces/surface.h"

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <map>
#include <memory>
#include <mutex>

namespace ms = mir::surfaces;

namespace
{
class NullSurfacesReport : public ms::SurfacesReport
{
public:
    virtual void surface_created(ms::Surface* const /*surface*/) {}
    virtual void create_surface_call(ms::Surface* const /*surface*/) {}

    virtual void delete_surface_call(ms::Surface* const /*surface*/) {}
    virtual void surface_deleted(ms::Surface* const /*surface*/) {}
};

class DebugSurfacesReport : public ms::SurfacesReport
{
public:
    virtual void surface_created(ms::Surface* const /*surface*/);
    virtual void create_surface_call(ms::Surface* const /*surface*/);

    virtual void delete_surface_call(ms::Surface* const /*surface*/);
    virtual void surface_deleted(ms::Surface* const /*surface*/);

private:
    std::mutex mutable mutex;

    std::map<ms::Surface*, std::string> surfaces;
};

bool is_debug()
{
    auto env = std::getenv("MIR_DEBUG_SURFACES_REPORT");
    return env && !std::strcmp(env, "debug");
}
}


void DebugSurfacesReport::surface_created(ms::Surface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);
    surfaces[surface] = surface->name();

    std::cout << "DebugSurfacesReport::surface_created(" <<
        surface << " [\"" << surface->name() << "\"])" << std::endl;
}

void DebugSurfacesReport::create_surface_call(ms::Surface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(surface);

    std::cout << "DebugSurfacesReport::create_surface_call(" <<
        surface << " [\"" << surface->name() << "\"])";

    if (i == surfaces.end())
    {
        std::cout << " - ERROR not reported to surface_created()";
    }
    else if (surface->name() != i->second)
    {
        std::cout << " - WARNING name changed from " << i->second;
    }

    std::cout << " - INFO surface count=" << surfaces.size() << std::endl;
}

void DebugSurfacesReport::delete_surface_call(ms::Surface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(surface);

    std::cout << "DebugSurfacesReport::delete_surface_call(" <<
        surface << " [\"" << surface->name() << "\"])";

    if (i == surfaces.end())
    {
        std::cout << " - ERROR not reported to surface_created()";
    }
    else if (surface->name() != i->second)
    {
        std::cout << " - WARNING name changed from " << i->second;
    }

    std::cout << " - INFO surface count=" << surfaces.size() << std::endl;
}

void DebugSurfacesReport::surface_deleted(ms::Surface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(surface);

    std::cout << "DebugSurfacesReport::surface_deleted(" <<
        surface << " [\"" << surface->name() << "\"])";

    if (i == surfaces.end())
    {
        std::cout << " - ERROR not reported to surface_created()";
    }
    else if (surface->name() != i->second)
    {
        std::cout << " - WARNING name changed from " << i->second;
    }

    if (i != surfaces.end())
        surfaces.erase(i);

    std::cout << " - INFO surface count=" << surfaces.size() << std::endl;
}

ms::SurfacesReport* ms::the_surfaces_report()
{
    static auto const result =
        is_debug() ?
            std::shared_ptr<ms::SurfacesReport>(std::make_shared<DebugSurfacesReport>()) :
            std::shared_ptr<ms::SurfacesReport>(std::make_shared<NullSurfacesReport>());

    return result.get();
}



