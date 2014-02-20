/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */


#ifndef MIR_REPORT_LTTNG_SCENE_REPORT_H_
#define MIR_REPORT_LTTNG_SCENE_REPORT_H_

#include "server_tracepoint_provider.h"

#include "mir/scene/scene_report.h"

namespace mir
{
namespace report
{
namespace lttng
{

class SceneReport : public scene::SceneReport
{
public:
    void surface_created(BasicSurfaceId id, std::string const& name) override;
    void surface_added(BasicSurfaceId id, std::string const& name) override;
    void surface_removed(BasicSurfaceId id, std::string const& name) override;
    void surface_deleted(BasicSurfaceId id, std::string const& name) override;
private:
    ServerTracepointProvider tp_provider;
};
}
}
}

#endif /* MIR_REPORT_LTTNG_SCENE_REPORT_H_ */
