/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#ifndef MIR_EVENT_BUILDERS_H_
#define MIR_EVENT_BUILDERS_H_

#include "mir_toolkit/event.h"

#include "mir/geometry/size.h"
#include "mir/frontend/surface_id.h"

#include <memory>

namespace mir
{
namespace events
{
// Surface orientation change event
std::shared_ptr<MirEvent> make_event(frontend::SurfaceId const& surface_id, MirOrientation orientation);
// Prompt session state change event
std::shared_ptr<MirEvent> make_event(MirPromptSessionState state);
// Surface resize event
std::shared_ptr<MirEvent> make_event(frontend::SurfaceId const& surface_id, geometry::Size const& size);
// Surface configure event
std::shared_ptr<MirEvent> make_event(frontend::SurfaceId const& surface_id, MirSurfaceAttrib attribute, int value);
// Close surface event
std::shared_ptr<MirEvent> make_event(frontend::SurfaceId const& surface_id);
}
}

#endif // MIR_EVENT_BUILDERS_H_
