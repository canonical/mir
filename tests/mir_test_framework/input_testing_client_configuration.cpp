/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>n
 */

#include "mir_test_framework/input_testing_client_configuration.h"
#include "mir_test_framework/testing_process_manager.h"

#include "mir_toolkit/mir_client_library.h"

#include <stdexcept>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

mtf::InputTestingClientConfiguration::InputTestingClientConfiguration(
    std::string const& client_name,
    mtf::CrossProcessSync const& input_cb_setup_fence) :
    client_name(client_name),
    input_cb_setup_fence(input_cb_setup_fence)
{
}

namespace
{
void handle_input(MirSurface* /* surface */, MirEvent const* ev, void* context)
{
    if (ev->type == mir_event_type_surface)
        return;

    auto handler = static_cast<mtf::InputTestingClientConfiguration::MockInputHandler*>(context);
    handler->handle_input(ev);
}
}

void mtf::InputTestingClientConfiguration::exec()
{
    mt::WaitCondition events_received;
    MockInputHandler handler;
    
    expect_input(handler, events_received);

    auto connection = mir_connect_sync(
        mtf::test_socket_file().c_str(), client_name.c_str());
    ASSERT_TRUE(connection != NULL);

    MirSurfaceParameters const request_params =
    {
        client_name.c_str(),
        surface_width, surface_height,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };
    auto surface = mir_connection_create_surface_sync(connection, &request_params);
    MirEventDelegate const event_delegate =
        {
            handle_input,
            &handler
        };
         // Set this in the callback, not main thread to avoid missing test events
    mir_surface_set_event_handler(surface, &event_delegate);
         
    try
    {
        input_cb_setup_fence.try_signal_ready_for();
    } 
    catch (const std::runtime_error& e)
    {
        std::cout << e.what() << std::endl;
    }
    events_received.wait_for_at_most_seconds(60);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}
    
