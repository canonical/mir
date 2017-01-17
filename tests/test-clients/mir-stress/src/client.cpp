/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Thomi Richards <thomi.richards@canonical.com>
 */

#include "client.h"

#include "mir_toolkit/mir_client_library.h"

#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>


ClientStateMachine::Ptr ClientStateMachine::Create()
{
    // in the future we can extend this to return different
    // types of clients. Right now we only return an unaccelerated client.
    return std::make_shared<UnacceleratedClient>();
}

UnacceleratedClient::UnacceleratedClient()
: connection_(nullptr)
, surface_(nullptr)
{
}

bool UnacceleratedClient::connect(std::string unique_name, const char* socket_file)
{
    assert(connection_ == nullptr);

    connection_ = mir_connect_sync(socket_file, unique_name.c_str());

    if (! connection_ || ! mir_connection_is_valid(connection_))
    {
        const char* error_message = mir_connection_get_error_message(connection_);
        if (std::strcmp(error_message, "") != 0)
        {
            std::cout << "Could not connect to server, error is: " << error_message << std::endl;
        }
        return false;
    }
    return true;
}

bool UnacceleratedClient::create_surface()
{
    auto display_configuration = mir_connection_create_display_configuration(connection_);
    auto num_outputs = mir_display_config_get_num_outputs(display_configuration);
    if (num_outputs < 1)
        return false;

    auto output = mir_display_config_get_output(display_configuration, 0);
    if (mir_output_get_num_pixel_formats(output) < 1)
        return false;

    // TODO: instead of picking the first pixel format, pick a random one!
    MirPixelFormat const pixel_format = mir_output_get_pixel_format(output, 0);
    mir_display_config_release(display_configuration);

    auto const spec = mir_create_normal_window_spec(connection_, 640, 480);
    mir_window_spec_set_pixel_format(spec, pixel_format);
    mir_window_spec_set_name(spec, __PRETTY_FUNCTION__);
    mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);

    surface_ = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    return true;
}

void UnacceleratedClient::release_surface()
{
    if (surface_ != nullptr)
    {
        mir_window_release_sync(surface_);
        surface_ = nullptr;
    }
}

void UnacceleratedClient::disconnect()
{
    if (connection_ != nullptr)
    {
        mir_connection_release(connection_);
        connection_ = nullptr;
    }
}
