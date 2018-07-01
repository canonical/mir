/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "touch_producing_server.h"
#include "vsync_simulating_graphics_platform.h"
#include "mir/input/input_device_info.h"

#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir/geometry/displacement.h"

#include <functional>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace mis = mi::synthesis;
namespace geom = mir::geometry;
namespace mt = mir::test;

namespace mtf = mir_test_framework;

TouchProducingServer::TouchProducingServer(geom::Rectangle screen_dimensions, geom::Point touch_start,
    geom::Point touch_end, std::chrono::high_resolution_clock::duration touch_duration,
    mt::Barrier &client_ready)
    : FakeInputServerConfiguration({screen_dimensions}),
      screen_dimensions(screen_dimensions),
      touch_start(touch_start),
      touch_end(touch_end),
      touch_duration(touch_duration),
      client_ready(client_ready),
      touch_screen(mtf::add_fake_input_device(mi::InputDeviceInfo{
                                              "touch screen", "touch-screen-uid", mi::DeviceCapability::touchscreen | mi::DeviceCapability::multitouch}))
{
    input_injection_thread = std::thread(std::mem_fn(&TouchProducingServer::thread_function), this);
}

TouchProducingServer::~TouchProducingServer()
{
    if (input_injection_thread.joinable())
        input_injection_thread.join();
}

std::shared_ptr<mg::Platform> TouchProducingServer::the_graphics_platform()
{
    // TODO: Support configuration
    int const refresh_rate_in_hz = 60;

    if (!graphics_platform)
        graphics_platform = std::make_shared<VsyncSimulatingPlatform>(screen_dimensions.size, refresh_rate_in_hz);
    
    return graphics_platform;
}

void TouchProducingServer::synthesize_event_at(geom::Point const& point)
{
    touch_screen->emit_event(mis::a_touch_event().at_position(point));
}

void TouchProducingServer::thread_function()
{
    // We could make the touch sampling rate customizable
    std::chrono::milliseconds const pause_between_events{10};

    client_ready.ready();
    
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + touch_duration;
    auto now = start;

    touch_start_time = std::chrono::high_resolution_clock::time_point::min();
    while (now < end)
    {
        std::this_thread::sleep_for(pause_between_events);

        now = std::chrono::high_resolution_clock::now();
        if (touch_start_time == std::chrono::high_resolution_clock::time_point::min())
            touch_start_time = now;
        touch_end_time = now;
        
        double alpha = (now.time_since_epoch().count()-start.time_since_epoch().count()) / static_cast<double>(end.time_since_epoch().count()-start.time_since_epoch().count());
        auto point = touch_start + alpha*(touch_end-touch_start);
        synthesize_event_at(point);
    }
}

TouchProducingServer::TouchTimings
TouchProducingServer::touch_timings()
{
    return {touch_start_time, touch_end_time};
}
