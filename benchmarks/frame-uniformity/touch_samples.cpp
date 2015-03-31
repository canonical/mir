/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "touch_samples.h"

void TouchSamples::record_frame_time(std::chrono::high_resolution_clock::time_point time)
{
    std::unique_lock<std::mutex> lg(guard);
    for (auto& sample: samples_being_prepared)
    {
        sample.frame_time = time;
        completed_samples.push_back(sample);
    }
    samples_being_prepared.clear();
}
    
void TouchSamples::record_pointer_coordinates(std::chrono::high_resolution_clock::time_point reception_time,
    MirEvent const& event)
{
    std::unique_lock<std::mutex> lg(guard);

    if (mir_event_get_type(&event) != mir_event_type_input)
        return;
    auto iev = mir_event_get_input_event(&event);
    if (mir_input_event_get_type(iev) != mir_input_event_type_touch)
        return;
    auto tev = mir_input_event_get_touch_event(iev);

    // We could support multitouch, etc...
    size_t touch_index = 0;
    auto action = mir_touch_event_action(tev, touch_index);
    if (action != mir_touch_action_down &&
        action != mir_touch_action_up &&
        action != mir_touch_action_change)
    {
        return;
    }
    auto x = mir_touch_event_axis_value(tev, 0, mir_touch_axis_x);
    auto y = mir_touch_event_axis_value(tev, 0, mir_touch_axis_y);
    // TODO: Record both event time and reception time
    samples_being_prepared.push_back(Sample{x, y, reception_time, {}});
}

std::vector<TouchSamples::Sample> TouchSamples::get()
{
    return completed_samples;
}
