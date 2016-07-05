/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "basic_seat.h"
#include "mir/input/device.h"
#include "mir/input/input_region.h"

#include <algorithm>

namespace mi = mir::input;
namespace mf = mir::frontend;

mi::BasicSeat::BasicSeat(std::shared_ptr<mi::InputDispatcher> const& dispatcher,
                         std::shared_ptr<mi::TouchVisualizer> const& touch_visualizer,
                         std::shared_ptr<mi::CursorListener> const& cursor_listener,
                         std::shared_ptr<mi::InputRegion> const& input_region,
                         std::shared_ptr<mi::KeyMapper> const& key_mapper)
    : input_state_tracker{dispatcher, touch_visualizer, cursor_listener, input_region, key_mapper}, input_region{input_region}
{
}

void mi::BasicSeat::add_device(input::Device const& device)
{
    input_state_tracker.add_device(device.id());
}

void mi::BasicSeat::remove_device(input::Device const& device)
{
    input_state_tracker.remove_device(device.id());
}

void mi::BasicSeat::dispatch_event(MirEvent& event)
{
    input_state_tracker.dispatch(event);
}

mir::geometry::Rectangle mi::BasicSeat::get_rectangle_for(input::Device const&)
{
    // TODO: With knowledge of the outputs attached to this seat and the output the given input
    // device is associated this method should only return the rectangle of that output. For now
    // we rely on the existing workaround in DisplayInputRegion::bounding_rectangle() which
    // assumes that only the first output may have a touch screen associated to it.
    return input_region->bounding_rectangle();
}

void mi::BasicSeat::set_confinement_regions(geometry::Rectangles const& regions)
{
    input_state_tracker.set_confinement_regions(regions);
}

void mi::BasicSeat::reset_confinement_regions()
{
    input_state_tracker.reset_confinement_regions();
}
