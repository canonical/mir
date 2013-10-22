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

#include "mir/default_server_configuration.h"

#include "mir/surfaces/surface_allocator.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/input/input_configuration.h"
#include "mir/surfaces/surfaces_report.h"

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace msh = mir::shell;


namespace
{
std::shared_ptr<ms::SurfacesReport> the_surfaces_report();
}

std::shared_ptr<ms::SurfaceStackModel>
mir::DefaultServerConfiguration::the_surface_stack_model()
{
    return surface_stack(
        [this]()
        {
            auto const surfaces_report = the_surfaces_report();

            auto const factory = std::make_shared<ms::SurfaceAllocator>(
                the_buffer_stream_factory(),
                the_input_channel_factory(),
                surfaces_report);

            auto const ss = std::make_shared<ms::SurfaceStack>(
                factory,
                the_input_registrar(),
                surfaces_report);

            the_input_configuration()->set_input_targets(ss);

            return ss;
        });
}

std::shared_ptr<mc::Scene>
mir::DefaultServerConfiguration::the_scene()
{
    return surface_stack(
        [this]()
        {
            auto const surfaces_report = the_surfaces_report();

            auto const factory = std::make_shared<ms::SurfaceAllocator>(
                the_buffer_stream_factory(),
                the_input_channel_factory(),
                surfaces_report);

            auto const ss = std::make_shared<ms::SurfaceStack>(
                factory,
                the_input_registrar(),
                surfaces_report);

            the_input_configuration()->set_input_targets(ss);

            return ss;
        });
}


std::shared_ptr<msh::SurfaceBuilder>
mir::DefaultServerConfiguration::the_surface_builder()
{
    return the_surface_controller();
}

std::shared_ptr<ms::SurfaceController>
mir::DefaultServerConfiguration::the_surface_controller()
{
    return surface_controller(
        [this]()
        {
            return std::make_shared<ms::SurfaceController>(the_surface_stack_model());
        });
}

std::shared_ptr<msh::SurfaceController>
mir::DefaultServerConfiguration::the_shell_surface_controller()
{
    return the_surface_controller();
}


///////////////////////////////
#include "mir/surfaces/surface.h"

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <map>
#include <mutex>

namespace
{
class DebugSurfacesReport : public ms::SurfacesReport
{
public:
    virtual void surface_created(ms::Surface* const /*surface*/);
    virtual void surface_added(ms::Surface* const /*surface*/);

    virtual void surface_removed(ms::Surface* const /*surface*/);
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

void DebugSurfacesReport::surface_added(ms::Surface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(surface);

    std::cout << "DebugSurfacesReport::surface_added(" <<
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

void DebugSurfacesReport::surface_removed(ms::Surface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(surface);

    std::cout << "DebugSurfacesReport::surface_removed(" <<
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

namespace
{
std::shared_ptr<ms::SurfacesReport> the_surfaces_report()
{
    static auto const result =
        is_debug() ?
            std::shared_ptr<ms::SurfacesReport>(std::make_shared<DebugSurfacesReport>()) :
            std::shared_ptr<ms::SurfacesReport>(std::make_shared<ms::NullSurfacesReport>());

    return result;
}
}
