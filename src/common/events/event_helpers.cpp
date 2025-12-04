/*
 * Copyright © 2023 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/events/event_helpers.h>
#include <mir/events/input_event.h>
#include <mir/events/pointer_event.h>
#include <mir/events/touch_event.h>

namespace mev = mir::events;
namespace geom = mir::geometry;

void mev::map_positions(MirEvent& event, MapPositionFunc const& func)
{
    if (event.type() == mir_event_type_input)
    {
        auto const input_type = event.to_input()->input_type();
        if (input_type == mir_input_event_type_pointer)
        {
            auto pev = event.to_input()->to_pointer();
            auto const global = pev->position();
            if (global)
            {
                auto const updated = func(global.value(), pev->local_position());
                pev->set_position(updated.first);
                pev->set_local_position(updated.second);
            }
        }
        else if (input_type == mir_input_event_type_touch)
        {
            auto tev = event.to_input()->to_touch();
            for (unsigned i = 0; i < tev->pointer_count(); i++)
            {
                auto const updated = func(tev->position(i), tev->local_position(i));
                tev->set_position(i, updated.first);
                tev->set_local_position(i, updated.second);
            }
        }
    }
}
