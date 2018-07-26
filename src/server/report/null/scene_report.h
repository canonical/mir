/*
 * Copyright © 2013, 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */


#ifndef MIR_REPORT_NULL_SCENE_REPORT_H_
#define MIR_REPORT_NULL_SCENE_REPORT_H_

#include "mir/scene/scene_report.h"

namespace mir
{
namespace report
{
namespace null
{

class SceneReport : public scene::SceneReport
{
public:
    virtual void surface_created(BasicSurfaceId /*id*/, std::string const& /*name*/) override;
    virtual void surface_added(BasicSurfaceId /*id*/, std::string const& /*name*/) override;

    virtual void surface_removed(BasicSurfaceId /*id*/, std::string const& /*name*/) override;
    virtual void surface_deleted(BasicSurfaceId /*id*/, std::string const& /*name*/) override;

    SceneReport() = default;
    virtual ~SceneReport() noexcept(true) = default;

};
}
}
}


#endif /* MIR_REPORT_NULL_SCENE_REPORT_H_ */
