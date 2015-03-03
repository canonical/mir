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
#include "mir/logging/input_timestamp.h"

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

#define CASE(action) case action: return out << #action
namespace
{
std::ostream& operator<<(std::ostream& out, MirKeyboardAction action)
{
    switch (action)
    {
    CASE(mir_keyboard_action_up);
    CASE(mir_keyboard_action_down);
    CASE(mir_keyboard_action_repeat);
    default:
        out << static_cast<int>(action) << "<INVALID>";
        return out;
    }
}

std::ostream& operator<<(std::ostream& out, MirTouchAction action)
{
    switch (action)
    {
    CASE(mir_touch_action_up);
    CASE(mir_touch_action_down);
    CASE(mir_touch_action_change);
    default:
        out << static_cast<int>(action) << "<INVALID>";
        return out;
    }
}

std::ostream& operator<<(std::ostream& out, MirPointerAction action)
{
    switch (action)
    {
    CASE(mir_pointer_action_button_up);
    CASE(mir_pointer_action_button_down);
    CASE(mir_pointer_action_enter);
    CASE(mir_pointer_action_leave);
    CASE(mir_pointer_action_motion);
    default:
        out << static_cast<int>(action) << "<INVALID>";
        return out;
    }
}

void format_key_event(std::stringstream &ss, MirInputEvent const* ev)
{
    auto kev = mir_input_event_get_keyboard_event(ev);
    
    ss << "MirKeyboardEvent {" << std::endl;
    ss << "  device_id: " << mir_input_event_get_device_id(ev) << std::endl;
    ss << "  action: " << mir_keyboard_event_action(kev) << std::endl;
    ss << "  modifiers: " << mir_keyboard_event_modifiers(kev) << std::endl;
    ss << "  key_code: " << mir_keyboard_event_key_code(kev) << std::endl;
    ss << "  scan_code: " << mir_keyboard_event_scan_code(kev) << std::endl;
    ss << "  event_time: " << ml::input_timestamp(std::chrono::nanoseconds(mir_input_event_get_event_time(ev))) << std::endl;
    ss << "}";
}

void format_touch_event(std::stringstream &ss, MirInputEvent const* ev)
{
    auto tev = mir_input_event_get_touch_event(ev);
    
    ss << "MirTouchEvent{" << std::endl;
    ss << "  type: touch" << std::endl;
    ss << "  device_id: " << mir_input_event_get_device_id(ev) << std::endl;
    ss << "  modifiers: " << mir_touch_event_modifiers(tev) << std::endl;
    ss << "  event_time: " << ml::input_timestamp(std::chrono::nanoseconds(mir_input_event_get_event_time(ev))) << std::endl;
    auto touch_count = mir_touch_event_point_count(tev);
    ss << "  touch_count: " << touch_count << std::endl;
    for (unsigned int i = 0; i < touch_count; i++)
    {
        ss << "  touch[" << i << "]{" << std::endl;
        ss << "    id: " << mir_touch_event_id(tev, i) << std::endl;
        ss << "    x: " << mir_touch_event_axis_value(tev, i, mir_touch_axis_x) << std::endl;
        ss << "    y: " <<  mir_touch_event_axis_value(tev, i, mir_touch_axis_y) << std::endl;
        ss << "    action: " << mir_touch_event_action(tev, i) << std::endl;
        ss << "    touch_major: " <<  mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_major) << std::endl;
        ss << "    touch_minor: " <<  mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_minor) << std::endl;
        ss << "    size: " <<  mir_touch_event_axis_value(tev, i, mir_touch_axis_size) << std::endl;
        ss << "    pressure: " <<  mir_touch_event_axis_value(tev, i, mir_touch_axis_pressure) << std::endl;
        ss << "    tool_type: " << mir_touch_event_tooltype(tev, i) << std::endl;
        ss << "  }" << std::endl;
    }
    ss << "}";
}

void format_pointer_event(std::stringstream &ss, MirInputEvent const* ev)
{
    auto pev = mir_input_event_get_pointer_event(ev);

    // TODO: Could be expanded
    ss << "MirPointerEvent {" << std::endl;
    ss << "  device_id: " << mir_input_event_get_device_id(ev) << std::endl;
    ss << "  action: " << mir_pointer_event_action(pev) << std::endl;
    ss << "  event_time: " << ml::input_timestamp(std::chrono::nanoseconds(mir_input_event_get_event_time(ev))) << std::endl;
    ss << "}";
}

void format_event(std::stringstream &ss, MirEvent const& ev)
{
    if (mir_event_get_type(&ev) != mir_event_type_input)
        return;
    auto iev = mir_event_get_input_event(&ev);
    switch (mir_input_event_get_type(iev))
    {
    case mir_input_event_type_key:
        format_key_event(ss, iev);
        break;
    case mir_input_event_type_touch:
        format_touch_event(ss, iev);
        break;
    case mir_input_event_type_pointer:
        format_pointer_event(ss, iev);
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

    logger->log(ml::Severity::debug, ss.str(), component);
}
