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

#include "touch_measuring_client.h"

#include "mir/time/clock.h"

#include "mir_toolkit/mir_client_library.h"

#include <iostream>

namespace mt = mir::test;

namespace
{

MirSurface *create_surface(MirConnection *connection)
{
    MirPixelFormat pixel_format;
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &valid_formats);

    auto const spec = mir_connection_create_spec_for_normal_surface(
        connection, 1024, 1024, pixel_format);
    mir_surface_spec_set_name(spec, "frame-uniformity-test");
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);

    auto surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);
    
    if (!mir_surface_is_valid(surface))
    {
        std::cerr << "Surface creation failed: " << mir_surface_get_error_message(surface) << std::endl;
        exit(1);
    }

    return surface;
}

void input_callback(MirSurface * /* surface */, MirEvent const* event, void* context)
{
    auto results = static_cast<TouchSamples*>(context);
    
    results->record_pointer_coordinates(std::chrono::high_resolution_clock::now(), *event);
}

void collect_input_and_frame_timing(MirSurface *surface, mt::Barrier& client_ready, std::chrono::high_resolution_clock::duration duration, std::shared_ptr<TouchSamples> const& results)
{
    mir_surface_set_event_handler(surface, input_callback, results.get());
    
    client_ready.ready();

    // May be better if end time were relative to the first input event
    auto end_time = std::chrono::high_resolution_clock::now() + duration;
    while (std::chrono::high_resolution_clock::now() < end_time)
    {
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
        results->record_frame_time(std::chrono::high_resolution_clock::now());
    }
}

}

TouchMeasuringClient::TouchMeasuringClient(mt::Barrier& client_ready,
    std::chrono::high_resolution_clock::duration const& touch_duration)
    : client_ready(client_ready),
      touch_duration(touch_duration),
      results_(std::make_shared<TouchSamples>())
{
}

namespace
{
void null_lifecycle_callback(MirConnection*, MirLifecycleState, void*)
{
}
}

void TouchMeasuringClient::run(std::string const& connect_string)
{
    auto connection = mir_connect_sync(connect_string.c_str(), "frame-uniformity-test");
    if (!mir_connection_is_valid(connection))
    {
        std::cerr << "Connection to Mir failed: " << mir_connection_get_error_message(connection) << std::endl;
        exit(1);
    }
    
    /*
     * Set a null callback to avoid killing the process
     * (default callback raises SIGHUP).
     */
    mir_connection_set_lifecycle_event_callback(connection, null_lifecycle_callback, nullptr);
    
    auto surface = create_surface(connection);

    collect_input_and_frame_timing(surface, client_ready, touch_duration, results_);
    
    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

std::shared_ptr<TouchSamples> TouchMeasuringClient::results()
{
    return results_;
}
