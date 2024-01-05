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

#ifndef MIR_INPUT_DEFAULT_EVENT_BUILDER_H_
#define MIR_INPUT_DEFAULT_EVENT_BUILDER_H_

#include "mir/input/event_builder.h"
#include <memory>
#include <atomic>

namespace mir
{
namespace time
{
class Clock;
}
namespace input
{
class Seat;

class DefaultEventBuilder : public EventBuilder
{
public:
    explicit DefaultEventBuilder(
        MirInputDeviceId device_id,
        std::shared_ptr<time::Clock> const& clock);

    EventUPtr key_event(
        std::optional<Timestamp> source_timestamp,
        MirKeyboardAction action,
        xkb_keysym_t keysym,
        int scan_code) override;

    EventUPtr touch_event(
        std::optional<Timestamp> source_timestamp,
        std::vector<events::TouchContactV1> const& contacts) override;

    EventUPtr pointer_event(
        std::optional<Timestamp> source_timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float hscroll_value, float vscroll_value,
        float relative_x_value, float relative_y_value) override;

    EventUPtr pointer_event(
        std::optional<Timestamp> source_timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float x, float y,
        float hscroll_value, float vscroll_value,
        float relative_x_value, float relative_y_value) override;

    EventUPtr pointer_axis_event(
        MirPointerAxisSource axis_source,
        std::optional<Timestamp> source_timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float x, float y,
        float hscroll_value, float vscroll_value,
        float relative_x_value, float relative_y_value) override;

    EventUPtr pointer_axis_with_stop_event(
        MirPointerAxisSource axis_source,
        std::optional<Timestamp> source_timestamp,
        MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float x, float y,
        float hscroll_value, float vscroll_value,
        bool hscroll_stop, bool vscroll_stop,
        float relative_x_value, float relative_y_value) override;

    EventUPtr pointer_axis_discrete_scroll_event(
        MirPointerAxisSource axis_source, std::optional<Timestamp> timestamp, MirPointerAction action,
        MirPointerButtons buttons_pressed, float hscroll_value, float vscroll_value, float hscroll_discrete,
        float vscroll_discrete) override;

    // Intentionally uses ScrollAxisV1* instad of ScrollAxis* as a reminder that a new copy of this function will be
    // needed for each ScrollAxis struct version.
    EventUPtr pointer_event(
        std::optional<Timestamp> timestamp,
        MirPointerAction action,
        MirPointerButtons buttons,
        std::optional<mir::geometry::PointF> position,
        mir::geometry::DisplacementF motion,
        MirPointerAxisSource axis_source,
        events::ScrollAxisV1H h_scroll,
        events::ScrollAxisV1V v_scroll) override;

    // Intentionally uses TouchContactV2* instad of TouchContact* as a reminder that a new copy of this function will be
    // needed for each TouchContact struct version.
    EventUPtr touch_event(
        std::optional<Timestamp> timestamp,
        std::vector<mir::events::TouchContactV2> const& contacts) override;

private:
    auto calibrate_timestamp(std::optional<Timestamp> source_timestamp) -> Timestamp;

    MirInputDeviceId const device_id;
    std::shared_ptr<time::Clock> const clock;
};
}
}

#endif
