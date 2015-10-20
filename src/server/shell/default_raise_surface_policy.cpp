/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/cookie_factory.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

#include "default_raise_surface_policy.h"

namespace ms = mir::scene;
namespace msh = mir::shell;

bool msh::DefaultRaiseSurfacePolicy::should_raise_surface(
    std::shared_ptr<ms::Surface> const& /* surface */,
    uint64_t timestamp) const
{
    return timestamp >= last_input_event_timestamp;
}

bool msh::DefaultRaiseSurfacePolicy::handle(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;

    auto const iev = mir_event_get_input_event(&event);
    auto iev_type  = mir_input_event_get_type(iev);

    switch (iev_type)
    {
        case mir_input_event_type_key:
            last_input_event_timestamp = mir_input_event_get_event_time(iev);
            break;
        case mir_input_event_type_pointer:
        {
            auto pev = mir_input_event_get_pointer_event(iev);
            auto pointer_action = mir_pointer_event_action(pev);

            if (pointer_action == mir_pointer_action_button_up ||
                pointer_action == mir_pointer_action_button_down)
            {
                last_input_event_timestamp = mir_input_event_get_event_time(iev);
            }
            break;
        }
        case mir_input_event_type_touch:
        {
            auto tev = mir_input_event_get_touch_event(iev);
            auto touch_count = mir_touch_event_point_count(tev);
            for (unsigned i = 0; i < touch_count; i++)
            {
                auto touch_action = mir_touch_event_action(tev, i);
                if (touch_action == mir_touch_action_up ||
                       touch_action == mir_touch_action_down)
                {
                    last_input_event_timestamp = mir_input_event_get_event_time(iev);
                    break;
                }
            }
            break;
        }
    }

    /* We dont eat the event, just need to check it out */
    return false;
}
