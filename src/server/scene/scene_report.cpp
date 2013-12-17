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

#include "mir/scene/scene_report.h"
#include "scene_report.h"
#include "mir/logging/logger.h"
#include "basic_surface.h"

#include <sstream>

namespace ms = mir::scene;
namespace ml = mir::logging;
namespace ms = mir::scene;

void ms::NullSceneReport::surface_created(BasicSurface* const /*surface*/) {}
void ms::NullSceneReport::surface_added(BasicSurface* const /*surface*/) {}
void ms::NullSceneReport::surface_removed(BasicSurface* const /*surface*/) {}
void ms::NullSceneReport::surface_deleted(BasicSurface* const /*surface*/) {}

namespace
{
char const* const component = "scene";
}

ml::SceneReport::SceneReport(std::shared_ptr<Logger> const& logger) :
    logger(logger)
{
}

void ml::SceneReport::surface_created(ms::BasicSurface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);
    surfaces[surface] = surface->name();

    std::stringstream ss;
    ss << "surface_created(" << surface << " [\"" << surface->name() << "\"])";

    logger->log(Logger::informational, ss.str(), component);
}

void ml::SceneReport::surface_added(ms::BasicSurface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(surface);

    std::stringstream ss;
    ss << "surface_added(" << surface << " [\"" << surface->name() << "\"])";

    if (i == surfaces.end())
    {
        ss << " - ERROR not reported to surface_created()";
    }
    else if (surface->name() != i->second)
    {
        ss << " - WARNING name changed from " << i->second;
    }

    ss << " - INFO surface count=" << surfaces.size();

    logger->log(Logger::informational, ss.str(), component);
}

void ml::SceneReport::surface_removed(ms::BasicSurface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(surface);

    std::stringstream ss;
    ss << "surface_removed(" << surface << " [\"" << surface->name() << "\"])";

    if (i == surfaces.end())
    {
        ss << " - ERROR not reported to surface_created()";
    }
    else if (surface->name() != i->second)
    {
        ss << " - WARNING name changed from " << i->second;
    }

    ss << " - INFO surface count=" << surfaces.size();

    logger->log(Logger::informational, ss.str(), component);
}

void ml::SceneReport::surface_deleted(ms::BasicSurface* const surface)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(surface);

    std::stringstream ss;
    ss << "surface_deleted(" << surface << " [\"" << surface->name() << "\"])";

    if (i == surfaces.end())
    {
        ss << " - ERROR not reported to surface_created()";
    }
    else if (surface->name() != i->second)
    {
        ss << " - WARNING name changed from " << i->second;
    }

    if (i != surfaces.end())
        surfaces.erase(i);

    ss << " - INFO surface count=" << surfaces.size() << std::endl;

    logger->log(Logger::informational, ss.str(), component);
}
