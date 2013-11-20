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


#ifndef MIR_LOGGING_SCENE_REPORT_H_
#define MIR_LOGGING_SCENE_REPORT_H_

#include "mir/scene/scene_report.h"

#include <map>
#include <memory>
#include <mutex>

namespace mir
{
namespace logging
{
class Logger;

class SceneReport : public scene::SceneReport
{
public:
    SceneReport(std::shared_ptr<Logger> const& log);

    void surface_created(scene::BasicSurface* const surface);
    void surface_added(scene::BasicSurface* const surface);
    void surface_removed(scene::BasicSurface* const surface);
    void surface_deleted(scene::BasicSurface* const surface);

private:
    std::shared_ptr<Logger> const logger;

    std::mutex mutex;
    std::map<scene::BasicSurface*, std::string> surfaces;
};
}
}



#endif /* MIR_LOGGING_SCENE_REPORT_H_ */
