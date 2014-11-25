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

#ifndef TOUCH_SAMPLES_H_
#define TOUCH_SAMPLES_H_

#include <vector>
#include <chrono>
#include <mutex>

#include <mir_toolkit/event.h>

class TouchSamples
{
public:
    TouchSamples() = default;
    ~TouchSamples() = default;
    
    // A touch sample
    struct Sample
    {
        // Coordinates of the touch
        float x,y;
        // Time at which the event left the input device
        std::chrono::high_resolution_clock::time_point event_time;
        // Submission time of first frame after receipt of event, e.g.
        // the earliest the event could potentially arrive onscreen.
        std::chrono::high_resolution_clock::time_point frame_time;
    };
    std::vector<Sample> get();
        
    void record_frame_time(std::chrono::high_resolution_clock::time_point time);
    void record_pointer_coordinates(std::chrono::high_resolution_clock::time_point reception_time,
                                    MirEvent const& ev);
private:
    std::mutex guard;

    // In between frames we will accumulate partially completed samples (lacking frame time)
    // in the "samples_being_prepared" collection. At each frame time we will move them to
    // the completed samples collection.
    std::vector<Sample> samples_being_prepared;
    std::vector<Sample> completed_samples;
};

#endif // TOUCH_SAMPLES_H_
