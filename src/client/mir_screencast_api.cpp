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

#define MIR_LOG_COMPONENT "MirScreencastAPI"

#include "mir_toolkit/mir_screencast.h"
#include "mir_screencast.h"
#include "mir_connection.h"
#include "mir/raii.h"

#include "mir/uncaught.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace
{
void null_callback(MirScreencast*, void*) {}
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

        mir::geometry::Rectangle const region{
            {parameters->region.left, parameters->region.top},
            {parameters->region.width, parameters->region.height}
        };
        mir::geometry::Size const size{parameters->width, parameters->height};

        std::unique_ptr<MirScreencast> screencast_uptr{
            new MirScreencast{
                region,
                size,
                parameters->pixel_format,
                connection->display_server(),
                connection,
                null_callback, nullptr}};

        screencast_uptr->creation_wait_handle()->wait_for_all();

        if (screencast_uptr->valid())
        {
            screencast = screencast_uptr.get();
            screencast_uptr.release();
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }

    return screencast;
}

void mir_screencast_release_sync(MirScreencast* screencast)
{
    screencast->release(null_callback, nullptr)->wait_for_all();
    delete screencast;
}

MirBufferStream *mir_screencast_get_buffer_stream(MirScreencast *screencast)
try
{
    return reinterpret_cast<MirBufferStream*>(screencast->get_buffer_stream());
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}
