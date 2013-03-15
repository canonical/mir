/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_input_reader_policy.h"
#include "android_pointer_controller.h"

#include "mir/graphics/viewable_area.h"

namespace mia = mir::input::android;
namespace mg = mir::graphics;

mia::InputReaderPolicy::InputReaderPolicy(std::shared_ptr<mg::ViewableArea> const& viewable_area,
                                          std::shared_ptr<CursorListener> const& cursor_listener)
    : viewable_area(viewable_area),
      pointer_controller(new mia::PointerController(viewable_area, cursor_listener))
{
}

void mia::InputReaderPolicy::getReaderConfiguration(droidinput::InputReaderConfiguration* out_config)
{
    static int32_t const default_display_id = 0;
    static bool const is_external = false;
    static int32_t const default_display_orientation = droidinput::DISPLAY_ORIENTATION_0;

    auto bounds = viewable_area->view_area();
    auto width = bounds.size.width.as_float();
    auto height = bounds.size.height.as_float();

    out_config->setDisplayInfo(
        default_display_id,
        is_external,
        width,
        height,
        default_display_orientation);
}
droidinput::sp<droidinput::PointerControllerInterface> mia::InputReaderPolicy::obtainPointerController(int32_t /*device_id*/)
{
    return pointer_controller;
}
