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

#include "frame_uniformity_test.h"
#include "mir_test_framework/executable_path.h"
#include "mir/geometry/displacement.h"

#include <assert.h>
#include <cmath>

#include <chrono>
#include <iostream>

#include <gtest/gtest.h>

namespace geom = mir::geometry;
namespace mtf = mir_test_framework;

namespace
{

geom::Point interpolated_touch_at_time(geom::Point touch_start, geom::Point touch_end,
    std::chrono::high_resolution_clock::time_point touch_start_time,
    std::chrono::high_resolution_clock::time_point touch_end_time,
    std::chrono::high_resolution_clock::time_point interpolated_touch_time)
{
    assert(interpolated_touch_time > touch_start_time);

    double elapsed_interval = interpolated_touch_time.time_since_epoch().count() - touch_start_time.time_since_epoch().count();
    double total_interval = touch_end_time.time_since_epoch().count() -
        touch_start_time.time_since_epoch().count();

    double alpha = elapsed_interval / total_interval;
    
    return touch_start + alpha*(touch_end-touch_start);
}

double pixel_lag_for_sample_at_time(geom::Point touch_start_point, geom::Point touch_end_point,
    std::chrono::high_resolution_clock::time_point touch_start_time,
    std::chrono::high_resolution_clock::time_point touch_end_time,
    TouchSamples::Sample const& sample)
{
    auto expected_point = interpolated_touch_at_time(touch_start_point, touch_end_point, touch_start_time,
        touch_end_time, sample.frame_time);

    geom::Displacement const displacement{
        sample.x - expected_point.x.as_int(),
        sample.y - expected_point.y.as_int()};

    return std::sqrt(displacement.length_squared());
}

double compute_average_frame_offset(std::vector<TouchSamples::Sample> const& results,
    geom::Point touch_start_point, geom::Point touch_end_point,
    std::chrono::high_resolution_clock::time_point touch_start_time,
    std::chrono::high_resolution_clock::time_point touch_end_time)
{
    double sum = 0;
    for (auto const& sample : results)
    {
        auto distance = pixel_lag_for_sample_at_time(touch_start_point, touch_end_point, touch_start_time, 
            touch_end_time, sample);
        sum += distance;
    }
    return sum / results.size();
}

struct Results
{
    double average_pixel_offset;
    double frame_uniformity;
};

Results compute_frame_uniformity(std::vector<TouchSamples::Sample> const& results,
    geom::Point touch_start_point, geom::Point touch_end_point,
    std::chrono::high_resolution_clock::time_point touch_start_time,
    std::chrono::high_resolution_clock::time_point touch_end_time)
{
    auto average_pixel_offset = compute_average_frame_offset(results, touch_start_point, touch_end_point,
        touch_start_time, touch_end_time);
    
    double sum = 0;
    for (auto const& sample : results)
    {
        auto distance = pixel_lag_for_sample_at_time(touch_start_point, touch_end_point, touch_start_time, 
            touch_end_time, sample);
        sum += (distance-average_pixel_offset)*(distance-average_pixel_offset);
    }
    double uniformity = std::sqrt(sum/results.size());
    return {average_pixel_offset, uniformity};
}

}

// Main is inside a test to work around mir_test_framework 'issues' (e.g. mir_test_framework contains
// a main function).
TEST(FrameUniformity, average_frame_offset)
{
    geom::Size const screen_size{1024, 1024};
    geom::Point const touch_start_point{0, 0};
    geom::Point const touch_end_point{1024, 1024};
    std::chrono::milliseconds touch_duration{1000};
    
    int const run_count = 1;
    double average_lag = 0, average_uniformity = 0;

    // Ensure we load the correct platform libraries
    setenv("MIR_CLIENT_PLATFORM_PATH",
           (mtf::library_path() + "/client-modules").c_str(),
           true);
    
    for (int i = 0; i < run_count; i++)
    {
        FrameUniformityTest t({screen_size, touch_start_point, touch_end_point, touch_duration});

        t.run_test();
  
        auto touch_timings = t.server_timings();
        auto touch_start_time = touch_timings.touch_start;
        auto touch_end_time = touch_timings.touch_end;
        auto samples = t.client_results()->get();

        auto results = compute_frame_uniformity(samples, touch_start_point, touch_end_point,
            touch_start_time, touch_end_time);
        
        average_lag += results.average_pixel_offset;
        average_uniformity += results.frame_uniformity;
    }
    
    average_lag /= run_count;
    average_uniformity /= run_count;
    
    std::cout << "Average pixel lag: " << average_lag << "px" << std::endl;
    std::cout << "Frame Uniformity (smaller scores are more uniform): " << average_uniformity << "px per sample\n"
        << std::endl;
}
