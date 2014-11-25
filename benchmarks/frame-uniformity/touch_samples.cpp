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

    if (event.type != mir_event_type_motion)
        return;
    
    auto const& mev = event.motion;
    if (mev.action != mir_motion_action_down &&
        mev.action != mir_motion_action_up &&
        mev.action != mir_motion_action_move)
    {
        return;
    }
    // We could support multitouch, etc...
    auto const& coordinates = mev.pointer_coordinates[0];

    // TODO: Record both event time and reception time
    samples_being_prepared.push_back(Sample{coordinates.x, coordinates.y, reception_time, {}});
}

std::vector<TouchSamples::Sample> TouchSamples::get()
{
    return completed_samples;
}
