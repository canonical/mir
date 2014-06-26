/*
 * Copyright © 2012 Canonical Ltd.
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

#ifndef MIR_INPUT_ANDROID_POINTER_CONTROLLER_H__
#define MIR_INPUT_ANDROID_POINTER_CONTROLLER_H__

#include "dummy_android_pointer_controller.h"

#include "mir/input/cursor_listener.h"

#include <memory>
#include <mutex>

namespace mir
{
namespace input
{
class TouchVisualizer;
class InputRegion;

namespace android
{
class PointerController : public DummyPointerController
{
  public:
    explicit PointerController(std::shared_ptr<InputRegion> const& input_region,
        std::shared_ptr<CursorListener> const& cursor_listener,
        std::shared_ptr<TouchVisualizer> const& touch_visualizer);

    bool getBounds(float* out_min_x, float* out_min_y, float* out_max_x, float* out_max_y) const;
    void move(float delta_x, float delta_y);
    void setButtonState(int32_t button_state);
    int32_t getButtonState() const;
    void setPosition(float x, float y);
    void getPosition(float *out_x, float *out_y) const;

    void setSpots(const droidinput::PointerCoords* spot_coords, uint32_t spot_count);
    void clearSpots();

  private:
    bool get_bounds_locked(float *out_min_x, float* out_min_y, float* out_max_x, float* out_max_y) const;
    void notify_listener();
    // Could be a read/write mutex as this is a latency sensitive class.
    mutable std::mutex guard;
    int32_t state;
    float x, y;

    std::shared_ptr<InputRegion> const input_region;
    std::shared_ptr<CursorListener> cursor_listener;
    std::shared_ptr<TouchVisualizer> const touch_visualizer;
};
}
}
}

#endif // MIR_INPUT_ANDROID_POINTER_CONTROLLER_H__
