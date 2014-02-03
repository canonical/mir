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


#ifndef MIR_LTTNG_SCENE_REPORT_H_
#define MIR_LTTNG_SCENE_REPORT_H_

#include "mir/scene/scene_report.h"
#include "mir/lttng/server_tracepoint_provider.h"

namespace mir
{
namespace lttng
{

class SceneReport : public scene::SceneReport
{
public:
    void surface_created(scene::BasicSurface* const surface) override;
    void surface_added(scene::BasicSurface* const surface) override;
    void surface_removed(scene::BasicSurface* const surface) override;
    void surface_deleted(scene::BasicSurface* const surface) override;
private:
    ServerTracepointProvider tp_provider;
};
}
}



#endif /* MIR_LTTNG_SCENE_REPORT_H_ */
