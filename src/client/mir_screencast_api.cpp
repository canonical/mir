/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir_toolkit/mir_screencast.h"
#include "mir_screencast.h"
#include "mir_connection.h"
#include "mir/raii.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace
{

void null_callback(MirScreencast*, void*) {}

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

MirScreencast* mir_connection_create_screencast_sync(
    MirConnection* connection,
    MirScreencastParameters* parameters)
{
    if (!MirConnection::is_valid(connection))
        return nullptr;

    MirScreencast* screencast = nullptr;

    try
    {
        auto const config = mir::raii::deleter_for(
            mir_connection_create_display_config(connection),
            &mir_display_config_destroy);

        auto const client_platform = connection->get_client_platform();

        std::unique_ptr<MirScreencast> screencast_uptr{
            new MirScreencast{
                find_display_output(*config, parameters->output_id),
                connection->display_server(),
                client_platform,
                client_platform->create_buffer_factory(),
                null_callback, nullptr}};

        screencast_uptr->creation_wait_handle()->wait_for_all();

        if (screencast_uptr->valid())
        {
            screencast = screencast_uptr.get();
            screencast_uptr.release();
        }
    }
    catch (std::exception const&)
    {
        return nullptr;
    }

    return screencast;
}

void mir_screencast_release_sync(MirScreencast* screencast)
{
    screencast->release(null_callback, nullptr)->wait_for_all();
    delete screencast;
}

MirEGLNativeWindowType mir_screencast_egl_native_window(MirScreencast* screencast)
{
    return screencast->egl_native_window();
}
