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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_output_capture.h"
#include "mir_output_capture.h"
#include "mir_connection.h"
#include "mir/raii.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace
{

void assign_result_callback(MirOutputCapture* result, void* context)
{
    auto capture = static_cast<MirOutputCapture**>(context);
    if (capture)
        *capture = result;
}

void null_callback(MirOutputCapture*, void*) {}

MirDisplayOutput& find_display_output(
    MirDisplayConfiguration const& config,
    uint32_t output_id)
{
    for (decltype(config.num_outputs) i = 0; i != config.num_outputs; ++i)
    {
        if (config.outputs[i].output_id == output_id)
        {
            return config.outputs[i];
        }
    }

    BOOST_THROW_EXCEPTION(
        std::runtime_error("Couldn't find output with supplied id"));
}

}

MirOutputCapture* mir_connection_create_output_capture_sync(
    MirConnection* connection,
    MirOutputCaptureParameters* parameters)
{
    if (!MirConnection::is_valid(connection))
        return nullptr;

    MirOutputCapture* capture = nullptr;

    try
    {
        auto const config = mir::raii::deleter_for(
            mir_connection_create_display_config(connection),
            &mir_display_config_destroy);

        auto const client_platform = connection->get_client_platform();

        capture = new MirOutputCapture{
            find_display_output(*config, parameters->output_id),
            connection->display_server(),
            client_platform,
            client_platform->create_buffer_factory(),
            assign_result_callback, &capture};

        capture->creation_wait_handle()->wait_for_all();
    }
    catch (std::exception const&)
    {
        return nullptr;
    }

    return capture;
}

void mir_output_capture_release_sync(MirOutputCapture* capture)
{
    capture->release(null_callback, nullptr)->wait_for_all();
    delete capture;
}

MirEGLNativeWindowType mir_output_capture_egl_native_window(MirOutputCapture* capture)
{
    return capture->egl_native_window();
}
