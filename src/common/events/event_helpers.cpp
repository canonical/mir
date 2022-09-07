/*
 * Copyright © 2022 Canonical Ltd.
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

#include "mir/events/event_helpers.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/events/touch_event.h"

namespace mev = mir::events;
namespace geom = mir::geometry;

void mev::set_local_position(MirEvent& event, mir::geometry::DisplacementF const& offset_from_global)
{
    if (event.type() == mir_event_type_input)
    {
        auto const input_type = event.to_input()->input_type();
        if (input_type == mir_input_event_type_pointer)
        {
            auto pev = event.to_input()->to_pointer();
            if (auto const position = pev->position())
            {
                pev->set_local_position(position.value() - offset_from_global);
            }
        }
        else if (input_type == mir_input_event_type_touch)
        {
            auto tev = event.to_input()->to_touch();
            for (unsigned i = 0; i < tev->pointer_count(); i++)
            {
                tev->set_local_position(i, tev->position(i) - offset_from_global);
            }
        }
    }
}

void mev::scale_local_position(MirEvent& event, float scale)
{
    if (event.type() == mir_event_type_input)
    {
        auto const input_type = event.to_input()->input_type();
        if (input_type == mir_input_event_type_pointer)
        {
            auto pev = event.to_input()->to_pointer();
            if (auto const local_position = pev->local_position())
            {
                pev->set_local_position(as_point(as_displacement(local_position.value()) * scale));
            }
        }
        else if (input_type == mir_input_event_type_touch)
        {
            auto tev = event.to_input()->to_touch();
            for (unsigned i = 0; i < tev->pointer_count(); i++)
            {
                if (auto const local_position = tev->local_position(i))
                {
                    tev->set_local_position(i, as_point(as_displacement(local_position.value()) * scale));
                }
            }
        }
    }
}
