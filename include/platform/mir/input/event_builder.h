/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_EVENT_BUILDER_H_
#define MIR_INPUT_EVENT_BUILDER_H_

#include <mir_toolkit/event.h>
#include <mir/events/touch_contact.h>
#include <mir/events/scroll_axis.h>
#include <memory>
#include <chrono>
#include <vector>
#include <optional>

namespace mir
{

using EventUPtr = std::unique_ptr<MirEvent, void(*)(MirEvent*)>;

namespace input
{
class EventBuilder
{
public:
    EventBuilder() = default;
    virtual ~EventBuilder() = default;
    /// Timestamps in returned events are automatically calibrated to the Mir clock. If nullopt is given current mir
    /// clock time is used.
    using Timestamp = std::chrono::nanoseconds;

    virtual EventUPtr key_event(
        std::optional<Timestamp> timestamp,
        MirKeyboardAction action,
        xkb_keysym_t keysym,
        int scan_code) = 0;

    [[deprecated("use the pointer_event() that includes all properties instead")]]
    virtual EventUPtr pointer_event(
        std::optional<Timestamp> timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float hscroll_value, float vscroll_value,
        float relative_x_value, float relative_y_value) = 0;

    [[deprecated("use the pointer_event() that includes all properties instead")]]
    virtual EventUPtr pointer_event(
        std::optional<Timestamp> timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float x_position, float y_position,
        float hscroll_value, float vscroll_value,
        float relative_x_value, float relative_y_value) = 0;

    [[deprecated("use the pointer_event() that includes all properties instead")]]
    virtual EventUPtr pointer_axis_event(
        MirPointerAxisSource axis_source,
        std::optional<Timestamp> timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float x_position, float y_position,
        float hscroll_value, float vscroll_value,
        float relative_x_value, float relative_y_value) = 0;

    [[deprecated("use the pointer_event() that includes all properties instead")]]
    virtual EventUPtr pointer_axis_discrete_scroll_event(
        MirPointerAxisSource axis_source,
        std::optional<Timestamp> timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float hscroll_value, float vscroll_value,
        float hscroll_discrete, float vscroll_discrete) = 0;

    [[deprecated("use the newest version of touch_event() instead")]]
    virtual EventUPtr touch_event(
        std::optional<Timestamp> timestamp,
        std::vector<mir::events::TouchContactV1> const& contacts) = 0;

    [[deprecated("use the pointer_event() that includes all properties instead")]]
    virtual EventUPtr pointer_axis_with_stop_event(
        MirPointerAxisSource axis_source,
        std::optional<Timestamp> timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float x_position, float y_position,
        float hscroll_value, float vscroll_value,
        bool hscroll_stop, bool vscroll_stop,
        float relative_x_value, float relative_y_value) = 0;

    virtual EventUPtr pointer_event(
        std::optional<Timestamp> timestamp,
        MirPointerAction action,
        MirPointerButtons buttons,
        std::optional<mir::geometry::PointF> position,
        mir::geometry::DisplacementF motion,
        MirPointerAxisSource axis_source,
        events::ScrollAxisH h_scroll,
        events::ScrollAxisV v_scroll) = 0;

    virtual EventUPtr touch_event(
        std::optional<Timestamp> timestamp,
        std::vector<mir::events::TouchContact> const& contacts) = 0;

protected:
    EventBuilder(EventBuilder const&) = delete;
    EventBuilder& operator=(EventBuilder const&) = delete;
};
}
}

#endif
