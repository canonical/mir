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

#ifndef TOUCH_PRODUCING_SERVER_H_
#define TOUCH_PRODUCING_SERVER_H_

#include "mir_test_framework/fake_input_server_configuration.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir/test/barrier.h"

#include "mir/geometry/rectangle.h"
#include "mir/geometry/point.h"

#include <thread>

class TouchProducingServer : public mir_test_framework::FakeInputServerConfiguration
{
public:
    TouchProducingServer(mir::geometry::Rectangle screen_dimensions, mir::geometry::Point touch_start, mir::geometry::Point touch_end, std::chrono::high_resolution_clock::duration touch_duration, mir::test::Barrier& client_ready);
    
    struct TouchTimings {
        std::chrono::high_resolution_clock::time_point touch_start;
        std::chrono::high_resolution_clock::time_point touch_end;
    };
    TouchTimings touch_timings();
    
    std::shared_ptr<mir::graphics::Platform> the_graphics_platform() override;

    ~TouchProducingServer();    
private:
    mir::geometry::Rectangle const screen_dimensions;

    mir::geometry::Point const touch_start;
    mir::geometry::Point const touch_end;
    std::chrono::high_resolution_clock::duration const touch_duration;

    mir::test::Barrier& client_ready;
    
    std::thread input_injection_thread;
    
    std::chrono::high_resolution_clock::time_point touch_start_time;
    std::chrono::high_resolution_clock::time_point touch_end_time;
    
    std::shared_ptr<mir::graphics::Platform> graphics_platform;
    
    void synthesize_event_at(mir::geometry::Point const& point);
    void thread_function();

    std::unique_ptr<mir_test_framework::FakeInputDevice> const touch_screen;
};

#endif
