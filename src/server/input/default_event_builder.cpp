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

#include "default_event_builder.h"
#include "mir/time/clock.h"
#include "mir/input/seat.h"
#include "mir/events/event_builders.h"

#include <algorithm>

namespace me = mir::events;
namespace mi = mir::input;

mi::DefaultEventBuilder::DefaultEventBuilder(
    MirInputDeviceId device_id,
    std::shared_ptr<time::Clock> const& clock)
    : device_id(device_id),
      clock(clock)
{
}

mir::EventUPtr mi::DefaultEventBuilder::key_event(
    std::optional<Timestamp> source_timestamp,
    MirKeyboardAction action,
    xkb_keysym_t keysym,
    int scan_code)
{
    auto const timestamp = calibrate_timestamp(source_timestamp);
    return me::make_key_event(
        device_id, timestamp, action, keysym, scan_code, mir_input_event_modifier_none);
}

mir::EventUPtr mi::DefaultEventBuilder::pointer_event(
    std::optional<Timestamp> source_timestamp,
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value)
{
    const float x_axis_value = 0;
    const float y_axis_value = 0;
    auto const timestamp = calibrate_timestamp(source_timestamp);
    return me::make_pointer_event(
        device_id, timestamp, mir_input_event_modifier_none, action, buttons_pressed, x_axis_value,
        y_axis_value,
        hscroll_value, vscroll_value, relative_x_value, relative_y_value);
}

mir::EventUPtr mi::DefaultEventBuilder::pointer_event(
    std::optional<Timestamp> source_timestamp,
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis, float y_axis,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value)
{
    auto const timestamp = calibrate_timestamp(source_timestamp);
    return me::make_pointer_event(
        device_id, timestamp, mir_input_event_modifier_none, action, buttons_pressed, x_axis, y_axis,
        hscroll_value, vscroll_value, relative_x_value, relative_y_value);
}

mir::EventUPtr mi::DefaultEventBuilder::pointer_axis_event(
    MirPointerAxisSource axis_source,
    std::optional<Timestamp> source_timestamp,
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis, float y_axis,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value)
{
    auto const timestamp = calibrate_timestamp(source_timestamp);
    return me::make_pointer_axis_event(
        axis_source, device_id, timestamp, mir_input_event_modifier_none, action, buttons_pressed, x_axis,
        y_axis, hscroll_value, vscroll_value, relative_x_value, relative_y_value);
}

mir::EventUPtr mi::DefaultEventBuilder::pointer_axis_with_stop_event(
    MirPointerAxisSource axis_source,
    std::optional<Timestamp> source_timestamp,
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis, float y_axis,
    float hscroll_value, float vscroll_value,
    bool hscroll_stop, bool vscroll_stop,
    float relative_x_value, float relative_y_value)
{
    auto const timestamp = calibrate_timestamp(source_timestamp);
    return me::make_pointer_axis_with_stop_event(
        axis_source, device_id, timestamp, mir_input_event_modifier_none, action, buttons_pressed, x_axis,
        y_axis, hscroll_value, vscroll_value, hscroll_stop, vscroll_stop, relative_x_value, relative_y_value);
}

mir::EventUPtr mir::input::DefaultEventBuilder::pointer_axis_discrete_scroll_event(
    MirPointerAxisSource axis_source, std::optional<Timestamp> source_timestamp, MirPointerAction action,
    MirPointerButtons buttons_pressed, float hscroll_value, float vscroll_value, float hscroll_discrete,
    float vscroll_discrete)
{
    auto const timestamp = calibrate_timestamp(source_timestamp);
    return me::make_pointer_axis_discrete_scroll_event(
        axis_source, device_id, timestamp, mir_input_event_modifier_none, action, buttons_pressed,
        hscroll_value, vscroll_value, hscroll_discrete, vscroll_discrete);
}

mir::EventUPtr mir::input::DefaultEventBuilder::pointer_event(
    std::optional<Timestamp> source_timestamp,
    MirPointerAction action,
    MirPointerButtons buttons,
    std::optional<mir::geometry::PointF> position,
    mir::geometry::DisplacementF motion,
    MirPointerAxisSource axis_source,
    events::ScrollAxisV1H h_scroll,
    events::ScrollAxisV1V v_scroll)
{
    auto const timestamp = calibrate_timestamp(source_timestamp);
    return me::make_pointer_event(
        device_id,
        timestamp,
        mir_input_event_modifier_none,
        action,
        buttons,
        position,
        motion,
        axis_source,
        h_scroll,
        v_scroll);
}

mir::EventUPtr mi::DefaultEventBuilder::touch_event(
    std::optional<Timestamp> source_timestamp,
    std::vector<events::TouchContactV1> const& contacts)
{
    return touch_event(
        source_timestamp,
        std::vector<events::TouchContactV2>{begin(contacts), end(contacts)});
}

mir::EventUPtr mi::DefaultEventBuilder::touch_event(
    std::optional<Timestamp> source_timestamp,
    std::vector<events::TouchContactV2> const& contacts)
{
    auto const timestamp = calibrate_timestamp(source_timestamp);
    return me::make_touch_event(device_id, timestamp, mir_input_event_modifier_none, contacts);
}

auto mi::DefaultEventBuilder::calibrate_timestamp(std::optional<Timestamp> timestamp) -> Timestamp
{
    using namespace std::chrono_literals;

    auto const now = clock->now().time_since_epoch();
    /// Only use timestamp if it is within the last 10 seconds (otherwise, a time source other than CLOCK_MONOTONIC
    /// is probably being used, which we ignore)
    if (timestamp && timestamp.value() <= now && timestamp.value() >= now - 10s)
    {
        return timestamp.value();
    }
    else
    {
        return now;
    }
}
