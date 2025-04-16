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
#include "mir/graphics/display.h"
#include "mir/input/composite_event_filter.h"

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace msh = mir::shell;
namespace geom = mir::geometry;

class msh::BasicMagnificationManager::Self : public mi::EventFilter
{
public:
    explicit Self(std::shared_ptr<graphics::Display> const& display)
        : display(display) {}

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
        update();
        return false;
    }

    void update()
    {
        // I believe that we actually want to scale the scene elements themselves so that
        // the renderables resolve to the correct compositor (BasicSurface::generate_renderables).

        display->for_each_display_sync_group([this](mg::DisplaySyncGroup& group)
        {
            group.for_each_display_sink([this](mg::DisplaySink& sink)
            {
                (void)sink;
            });
        });
    }

    std::shared_ptr<graphics::Display> const display;
    bool enabled = false;
    float magnification = 1.f;
    geom::PointF position;
};

msh::BasicMagnificationManager::BasicMagnificationManager(
    std::shared_ptr<input::CompositeEventFilter> const& filter,
    std::shared_ptr<graphics::Display> const& display)
    : self(std::make_shared<Self>(display))
{
    filter->prepend(self);
}

void mir::shell::BasicMagnificationManager::enabled(bool enabled)
{
    bool const last_enabled = self->enabled;
    self->enabled = enabled;
    if (self->enabled != last_enabled)
        self->update();
}

void mir::shell::BasicMagnificationManager::magnification(float magnification)
{
    self->magnification = magnification;
    self->update();
}
