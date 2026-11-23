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

#ifndef MIR_INPUT_CURSOR_HIDING_DISPATCHER_H_
#define MIR_INPUT_CURSOR_HIDING_DISPATCHER_H_

#include <mir/input/cursor_hiding_policy.h>
#include <mir/input/input_dispatcher.h>
#include <mir/input/input_device_observer.h>
#include <mir/geometry/point.h>
#include <mir/time/types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <set>

namespace mir
{
namespace time
{
class Alarm;
class AlarmFactory;
class Clock;
}
namespace input
{
/// Applies the CursorHidingPolicy to events before passing them on to the next dispatcher
class CursorHidingDispatcher : public InputDispatcher, public InputDeviceObserver, public CursorHidingPolicy
{
public:
    CursorHidingDispatcher(
        std::shared_ptr<InputDispatcher> const& next_dispatcher,
        std::function<void(bool hidden)> set_cursor_hidden,
        std::function<bool()> pointer_constrained,
        std::shared_ptr<time::Clock> const& clock,
        time::AlarmFactory& alarm_factory);
    ~CursorHidingDispatcher();

    /// InputDispatcher overrides
    /// @{
    bool dispatch(std::shared_ptr<MirEvent const> const& event) override;
    void start() override;
    void stop() override;
    /// @}

    /// InputDeviceObserver overrides
    /// @{
    void device_added(std::shared_ptr<Device> const& device) override;
    void device_changed(std::shared_ptr<Device> const& device) override;
    void device_removed(std::shared_ptr<Device> const& device) override;
    void changes_complete() override;
    /// @}

    /// CursorHidingPolicy overrides
    /// @{
    void hide_on_touch(bool enabled) override;
    void hide_on_keyboard(bool enabled) override;
    void hide_after_inactivity(std::optional<std::chrono::milliseconds> timeout) override;
    void reveal_threshold(float distance) override;
    /// @}

private:
    class AlarmCallback;

    void handle_pointer(MirPointerEvent const* event);
    void hide_locked();
    void show_locked();
    void schedule_alarm_locked();
    void alarm_fired_locked();

    std::shared_ptr<InputDispatcher> const next_dispatcher;
    std::function<void(bool hidden)> const set_cursor_hidden;
    std::function<bool()> const pointer_constrained;
    std::shared_ptr<time::Clock> const clock;

    std::mutex mutex;
    bool touch_enabled{false};
    bool keyboard_enabled{false};
    std::optional<std::chrono::milliseconds> inactivity_timeout;
    float threshold;

    bool hidden{false};
    bool buttons_held{false};
    float distance_moved{0};
    std::optional<geometry::PointF> last_position;
    time::Timestamp last_pointer_activity;
    std::set<MirInputDeviceId> pointing_devices;
    // Declared last so that it is destroyed (and cannot fire) before the state above
    std::unique_ptr<time::Alarm> const alarm;
};
}
}

#endif // MIR_INPUT_CURSOR_HIDING_DISPATCHER_H_
