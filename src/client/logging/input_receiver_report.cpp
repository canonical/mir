/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "input_receiver_report.h"

#include "mir/logging/logger.h"

#include <boost/throw_exception.hpp>

#include <sstream>
#include <stdexcept>

namespace ml = mir::logging;
namespace mcll = mir::client::logging;

namespace
{
std::string const component{"input-receiver"};
}

mcll::InputReceiverReport::InputReceiverReport(std::shared_ptr<ml::Logger> const& logger)
    : logger{logger}
{
}

namespace
{

static void format_key_event(std::stringstream &ss, MirKeyEvent const& ev)
{
    ss << "MirKeyEvent {" << std::endl;
    ss << "  device_id: " << ev.device_id << std::endl;
    ss << "  source_id: " << ev.source_id << std::endl;
    ss << "  action: " << ev.action << std::endl;
    ss << "  flags: " << ev.flags << std::endl;
    ss << "  modifiers: " << ev.modifiers << std::endl;
    ss << "  key_code: " << ev.key_code << std::endl;
    ss << "  scan_code: " << ev.scan_code << std::endl;
    ss << "  repeat_count: " << ev.repeat_count << std::endl;
    ss << "  down_time: " << ev.down_time << std::endl;
    ss << "  event_time: " << ev.event_time << std::endl;
    ss << "  is_system_key: " << ev.is_system_key << std::endl;
    ss << "}";
}
static void format_motion_event(std::stringstream &ss, MirMotionEvent const& ev)
{
    ss << "MirMotionEvent{" << std::endl;
    ss << "  type: motion" << std::endl;
    ss << "  device_id: " << ev.device_id << std::endl;
    ss << "  source_id: " << ev.source_id << std::endl;
    ss << "  action: " << ev.action << std::endl;
    ss << "  flags: " << ev.flags << std::endl;
    ss << "  modifiers: " << ev.modifiers << std::endl;
    ss << "  edge_flags: " << ev.edge_flags << std::endl;
    ss << "  button_state: " << ev.button_state << std::endl;
    ss << "  x_offset: " << ev.x_offset << std::endl;
    ss << "  y_offset: " << ev.y_offset << std::endl;
    ss << "  x_precision: " << ev.x_precision << std::endl;
    ss << "  y_precision: " << ev.y_precision << std::endl;
    ss << "  down_time: " << ev.down_time << std::endl;
    ss << "  event_time: " << ev.event_time << std::endl;
    ss << "  pointer_count: " << ev.pointer_count << std::endl;
    for (unsigned int i = 0; i < ev.pointer_count; i++)
    {
        ss << "  pointer[" << i << "]{" << std::endl;
        ss << "    id: " << ev.pointer_coordinates[i].id << std::endl;
        ss << "    x: " << ev.pointer_coordinates[i].x << std::endl;
        ss << "    raw_x: " << ev.pointer_coordinates[i].raw_x << std::endl;
        ss << "    y: " << ev.pointer_coordinates[i].y << std::endl;
        ss << "    raw_y: " << ev.pointer_coordinates[i].raw_y << std::endl;
        ss << "    touch_major: " << ev.pointer_coordinates[i].touch_major << std::endl;
        ss << "    touch_minor: " << ev.pointer_coordinates[i].touch_minor << std::endl;
        ss << "    size: " << ev.pointer_coordinates[i].size << std::endl;
        ss << "    pressure: " << ev.pointer_coordinates[i].pressure << std::endl;
        ss << "    orientation: " << ev.pointer_coordinates[i].orientation << std::endl;
        ss << "    vscroll: " << ev.pointer_coordinates[i].vscroll << std::endl;
        ss << "    hscroll: " << ev.pointer_coordinates[i].hscroll << std::endl;
        ss << "  }" << std::endl;
    }
    ss << "}";
}

static void format_event(std::stringstream &ss, MirEvent const& ev)
{
    switch (ev.type)
    {
    case mir_event_type_key:
        format_key_event(ss, ev.key);
        break;
    case mir_event_type_motion:
        format_motion_event(ss, ev.motion);
        break;
    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("Unexpected event type"));
    }
}

}

void mcll::InputReceiverReport::received_event(
    MirEvent const& event)
{
    std::stringstream ss;

    ss << "Received event:" << std::endl;

    format_event(ss, event);

    logger->log<ml::Logger::debug>(ss.str(), component);
}
