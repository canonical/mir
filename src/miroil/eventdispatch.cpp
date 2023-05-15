/*
 * Copyright © Canonical Ltd.
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
 */

#include "miroil/eventdispatch.h"
#include <miral/window.h>
#include <mir/scene/surface.h>
#include <mir/events/event_builders.h>
#include <mir/events/input_event.h>
#include <mir/events/event_helpers.h>

namespace mev = mir::events;
namespace geom = mir::geometry;

void miroil::dispatch_input_event(const miral::Window& window, const MirInputEvent* event)
{
    if (auto surface = std::shared_ptr<mir::scene::Surface>(window))
    {
        std::shared_ptr<MirEvent> const clone = mev::clone_event(*event);
        mev::map_positions(*clone, [&](auto global, auto local)
            {
                if (!local)
                {
                    // If local position is not set, local position is confunsingly being stored in global positon. This
                    // is a remnant of before Mir stored local and global positon in separate variables within the
                    // event.
                    local = global;
                    std::shared_ptr<mir::scene::Surface> const surface{window};
                    auto surface_displacement = geom::DisplacementF{as_displacement(surface->input_bounds().top_left)};
                    global = local.value() + surface_displacement;
                }
                return std::make_pair(global, local);
            });
        surface->consume(clone);
    }
}
