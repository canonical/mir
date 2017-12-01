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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <sstream>

#include "seat_report.h"
#include "mir/events/event.h"
#include "mir/geometry/rectangles.h"
#include "mir/logging/logger.h"
#include "mir/input/device.h"

namespace mi   = mir::input;
namespace ml   = mir::logging;
namespace mrl  = mir::report::logging;
namespace geom = mir::geometry;

namespace
{
char const* const component = "input::Seat";

std::string scan_codes_to_string(std::vector<uint32_t> const& scan_codes)
{
    std::stringstream ss;
    ss << "{";

    for (auto const& s : scan_codes)
    {
        ss << s << ", ";
    }

    auto str = ss.str();

    // Remove the extra , and space
    str.pop_back();
    str.pop_back();

    return str + "}";
}

}

mrl::SeatReport::SeatReport(std::shared_ptr<ml::Logger> const& log) :
    log(log)
{
}

void mrl::SeatReport::seat_add_device(uint64_t id)
{
    std::stringstream ss;
    ss << "Add device"
       << " device_id=" << id;

    log->log(ml::Severity::informational, ss.str(), component);
}

void mrl::SeatReport::seat_remove_device(uint64_t id)
{
    std::stringstream ss;
    ss << "Remove device"
       << " device_id=" << id;

    log->log(ml::Severity::informational, ss.str(), component);
}

void mrl::SeatReport::seat_dispatch_event(std::shared_ptr<MirEvent const> const& event)
{
    std::stringstream ss;
    ss << "Dispatch event"
       << " event_type=" << event->type();

    log->log(ml::Severity::informational, ss.str(), component);
}

void mrl::SeatReport::seat_set_key_state(uint64_t id, std::vector<uint32_t> const& scan_codes)
{
    std::stringstream ss;
    ss << "Set key state"
       << " device_id="  << id
       << " scan_codes=" << scan_codes_to_string(scan_codes);

    log->log(ml::Severity::informational, ss.str(), component);
}

void mrl::SeatReport::seat_set_pointer_state(uint64_t id, unsigned buttons)
{
    std::stringstream ss;
    ss << "Set pointer state"
       << " devie_id=" << id
       << " buttons="  << buttons;

    log->log(ml::Severity::informational, ss.str(), component);
}

void mrl::SeatReport::seat_set_cursor_position(float cursor_x, float cursor_y)
{
    std::stringstream ss;
    ss << "Set cursor position"
       << " cursor_x=" << cursor_x
       << " cursor_y=" << cursor_y;

    log->log(ml::Severity::informational, ss.str(), component);
}

void mrl::SeatReport::seat_set_confinement_region_called(geom::Rectangles const& regions)
{
    std::stringstream ss;

    auto bound_rect = regions.bounding_rectangle();
    ss << "Set confinement region"
       << " regions=" << bound_rect;

    log->log(ml::Severity::informational, ss.str(), component);
}

void mrl::SeatReport::seat_reset_confinement_regions()
{
    std::stringstream ss;
    ss << "Reset confinement regions";

    log->log(ml::Severity::informational, ss.str(), component);
}
