/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "seat_observer_multiplexer.h"

#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"

namespace mi = mir::input;
namespace geom = mir::geometry;

void mi::SeatObserverMultiplexer::seat_add_device(uint64_t id)
{
    for_each_observer(&mi::SeatObserver::seat_add_device, id);
}

void mi::SeatObserverMultiplexer::seat_remove_device(uint64_t id)
{
    for_each_observer(&mi::SeatObserver::seat_remove_device, id);
}

void mi::SeatObserverMultiplexer::seat_dispatch_event(
    std::shared_ptr<MirEvent const> const& event)
{
    for_each_observer(&mi::SeatObserver::seat_dispatch_event, event);
}

void mi::SeatObserverMultiplexer::seat_get_rectangle_for(
    uint64_t id,
    geom::Rectangle const& out_rect)
{
    for_each_observer(&mi::SeatObserver::seat_get_rectangle_for, id, out_rect);
}

void mi::SeatObserverMultiplexer::seat_set_key_state(uint64_t id, std::vector<uint32_t> const& scan_codes)
{
    for_each_observer(&mi::SeatObserver::seat_set_key_state, id, scan_codes);
}

void mi::SeatObserverMultiplexer::seat_set_pointer_state(uint64_t id, unsigned buttons)
{
    for_each_observer(&mi::SeatObserver::seat_set_pointer_state, id, buttons);
}

void mi::SeatObserverMultiplexer::seat_set_cursor_position(float cursor_x, float cursor_y)
{
    for_each_observer(&mi::SeatObserver::seat_set_cursor_position, cursor_x, cursor_y);
}

void mi::SeatObserverMultiplexer::seat_set_confinement_region_called(
    geom::Rectangles const& regions)
{
    for_each_observer(&mi::SeatObserver::seat_set_confinement_region_called, regions);
}

void mi::SeatObserverMultiplexer::seat_reset_confinement_regions()
{
    for_each_observer(&mi::SeatObserver::seat_reset_confinement_regions);
}

mi::SeatObserverMultiplexer::SeatObserverMultiplexer(
    std::shared_ptr<mir::Executor> const& default_executor)
    : ObserverMultiplexer(*default_executor),
      executor{default_executor}
{
}
