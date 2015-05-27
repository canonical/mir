/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_
#define MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_

#include "mir/input/input_dispatcher.h"
#include "mir/shell/input_targeter.h"
#include "mir/geometry/point.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace mir
{
namespace scene
{
class Observer;
class Surface;
}
namespace input
{
class Surface;
class Scene;

class SurfaceInputDispatcher : public mir::input::InputDispatcher, public shell::InputTargeter
{
public:
    SurfaceInputDispatcher(std::shared_ptr<input::Scene> const& scene);
    ~SurfaceInputDispatcher();

    // mir::input::InputDispatcher
    bool dispatch(MirEvent const& event) override;
    void start() override;
    void stop() override;

    // InputTargeter
    void set_focus(std::shared_ptr<input::Surface> const& target) override;
    void clear_focus() override;
    
private:
    void device_reset(MirInputDeviceId reset_device_id, std::chrono::nanoseconds when);
    bool dispatch_key(MirInputDeviceId id, MirKeyboardEvent const* kev);
    bool dispatch_pointer(MirInputDeviceId id, MirPointerEvent const* pev);
    bool dispatch_touch(MirInputDeviceId id, MirTouchEvent const* tev);

    void send_enter_exit_event(std::shared_ptr<input::Surface> const& surface,
        MirPointerEvent const* triggering_ev, MirPointerAction action);

    std::shared_ptr<input::Surface> find_target_surface(geometry::Point const& target);

    void set_focus_locked(std::lock_guard<std::mutex> const&, std::shared_ptr<input::Surface> const&);

    void surface_removed(scene::Surface* surface);

    struct KeyInputState
    {
        bool handle_event(MirInputDeviceId id, MirKeyboardEvent const* kev);
        
        bool press_key(MirInputDeviceId id, int scan_code);
        bool repeat_key(MirInputDeviceId id, int scan_code);
        bool release_key(MirInputDeviceId id, int scan_code);

        void clear();

        std::unordered_map<MirInputDeviceId, std::unordered_set<int>> depressed_scancodes;
    } focus_surface_key_state;

    // Look in to homognizing index on KeyInputState and PointerInputState (wrt to device id)
    struct PointerInputState
    {
        std::shared_ptr<input::Surface> current_target;
        std::shared_ptr<input::Surface> gesture_owner;
    };
    std::unordered_map<MirInputDeviceId, PointerInputState> pointer_state_by_id;
    PointerInputState& ensure_pointer_state(MirInputDeviceId id);

    struct TouchInputState
    {
        std::shared_ptr<input::Surface> gesture_owner;
    };
    std::unordered_map<MirInputDeviceId, TouchInputState> touch_state_by_id;
    TouchInputState& ensure_touch_state(MirInputDeviceId id);
    
    std::shared_ptr<input::Scene> const scene;

    std::shared_ptr<scene::Observer> scene_observer;

    std::mutex dispatcher_mutex;
    std::weak_ptr<input::Surface> focus_surface;
    bool started;
};

}
}

#endif // MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_
