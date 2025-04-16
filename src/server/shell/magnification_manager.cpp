/*
* Copyright © Canonical Ltd.
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
 */

#include "magnification_manager.h"

#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/input/composite_event_filter.h"

namespace msh = mir::shell;
namespace mi = mir::input;
namespace geom = mir::geometry;

class msh::BasicMagnificationManager::Self : public mi::EventFilter
{
public:
    bool handle(MirEvent const& event) override
    {
        if (!enabled)
            return false;

        auto const* input_event = event.to_input();
        if (!input_event)
            return false;

        auto const* pointer_event = input_event->to_pointer();
        if (!pointer_event)
            return false;

        if (pointer_event->action() != mir_pointer_action_motion)
            return false;

        auto const position_opt = pointer_event->position();
        if (!position_opt)
            return false;

        position = position_opt.value();

        return false;
    }

    bool enabled = false;
    float magnification = 1.f;
    geom::PointF position;
};

msh::BasicMagnificationManager::BasicMagnificationManager(
    std::shared_ptr<input::CompositeEventFilter> const& filter)
    : self(std::make_shared<Self>())
{
    filter->prepend(self);
}

void mir::shell::BasicMagnificationManager::enabled(bool enabled)
{
    if (!self->enabled && enabled)
    {
        // TODO: Reset the zoom
    }
    self->enabled = enabled;
}

void mir::shell::BasicMagnificationManager::magnification(float magnification)
{
    self->magnification = magnification;
    // TODO: Update the zoom
}
