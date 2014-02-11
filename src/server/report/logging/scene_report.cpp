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

#include "scene_report.h"

#include "mir/logging/logger.h"

#include <sstream>

namespace ml = mir::logging;
namespace mrl = mir::report::logging;

namespace
{
char const* const component = "scene";
}

mrl::SceneReport::SceneReport(std::shared_ptr<ml::Logger> const& logger) :
    logger(logger)
{
}

void mrl::SceneReport::surface_created(BasicSurfaceId id, std::string const& name)
{
    std::lock_guard<std::mutex> lock(mutex);
    surfaces[id] = name;

    std::stringstream ss;
    ss << "surface_created(" << id << " [\"" << name << "\"])";

    logger->log(ml::Logger::informational, ss.str(), component);
}

void mrl::SceneReport::surface_added(BasicSurfaceId id, std::string const& name)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(id);

    std::stringstream ss;
    ss << "surface_added(" << id << " [\"" << name << "\"])";

    if (i == surfaces.end())
    {
        ss << " - ERROR not reported to surface_created()";
    }
    else if (name != i->second)
    {
        ss << " - WARNING name changed from " << i->second;
    }

    ss << " - INFO surface count=" << surfaces.size();

    logger->log(ml::Logger::informational, ss.str(), component);
}

void mrl::SceneReport::surface_removed(BasicSurfaceId id, std::string const& name)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(id);

    std::stringstream ss;
    ss << "surface_removed(" << id << " [\"" << name << "\"])";

    if (i == surfaces.end())
    {
        ss << " - ERROR not reported to surface_created()";
    }
    else if (name != i->second)
    {
        ss << " - WARNING name changed from " << i->second;
    }

    ss << " - INFO surface count=" << surfaces.size();

    logger->log(ml::Logger::informational, ss.str(), component);
}

void mrl::SceneReport::surface_deleted(BasicSurfaceId id, std::string const& name)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const i = surfaces.find(id);

    std::stringstream ss;
    ss << "surface_deleted(" << id << " [\"" << name << "\"])";

    if (i == surfaces.end())
    {
        ss << " - ERROR not reported to surface_created()";
    }
    else if (name != i->second)
    {
        ss << " - WARNING name changed from " << i->second;
    }

    if (i != surfaces.end())
        surfaces.erase(i);

    ss << " - INFO surface count=" << surfaces.size() << std::endl;

    logger->log(ml::Logger::informational, ss.str(), component);
}
