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

#include "android_input_reader_policy.h"
#include "android_pointer_controller.h"

#include "mir/input/input_region.h"
#include "mir/geometry/rectangle.h"

namespace mi = mir::input;
namespace mia = mir::input::android;

mia::InputReaderPolicy::InputReaderPolicy(std::shared_ptr<mi::InputRegion> const& input_region,
                                          std::shared_ptr<CursorListener> const& cursor_listener,
                                          std::shared_ptr<TouchVisualizer> const& touch_visualizer)
    : input_region(input_region),
      pointer_controller(new mia::PointerController(input_region, cursor_listener, touch_visualizer))
{
}

void mia::InputReaderPolicy::getReaderConfiguration(droidinput::InputReaderConfiguration* out_config)
{
    static int32_t const default_display_id = 0;
    static bool const is_external = false;
    static int32_t const default_display_orientation = droidinput::DISPLAY_ORIENTATION_0;

    auto bounds = input_region->bounding_rectangle();
    auto width = bounds.size.width.as_float();
    auto height = bounds.size.height.as_float();

    out_config->setDisplayInfo(
        default_display_id,
        is_external,
        width,
        height,
        default_display_orientation);

    out_config->pointerVelocityControlParameters.acceleration = 1.0;
    
    // This only enables passing through the touch coordinates from the InputReader to the TouchVisualizer
    // the touch visualizer still decides whether or not to render anything.
    out_config->showTouches = true;
}

droidinput::sp<droidinput::PointerControllerInterface> mia::InputReaderPolicy::obtainPointerController(int32_t /*device_id*/)
{
    return pointer_controller;
}

void mia::InputReaderPolicy::getAssociatedDisplayInfo(droidinput::InputDeviceIdentifier const& /* identifier */,
    int& out_associated_display_id, bool& out_associated_display_is_external)
{
    // This is used primarily for mapping of direct input devices to screen space. Currently we only support one large display
    // bounding all displays (i.e. input_region->bounding_rectangle())
    // the external/internal distinction is mostly a leftover from android configuration nuances.
    // Eventually we will need to use a combination of configuration and heuristic to implement this 
    // correctly
    out_associated_display_id = 0;
    out_associated_display_is_external = false;
}
