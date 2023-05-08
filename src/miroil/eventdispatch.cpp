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

void miroil::dispatch_input_event(const miral::Window& window, const MirInputEvent* event)
{
    if (auto surface = std::shared_ptr<mir::scene::Surface>(window))
    {
        std::shared_ptr<MirEvent> const clone = mir::events::clone_event(*event);
        // Unclear if this is the right thing to do
        mir::events::set_local_position_from_input_bounds_top_left(*clone, {});
        surface->consume(clone);
    }
}
