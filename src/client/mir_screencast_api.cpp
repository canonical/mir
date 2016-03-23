/*
 * Copyright © 2014,2016 Canonical Ltd.
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
#include "mir/require.h"
#include "mir/uncaught.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace
{
void null_callback(MirScreencast*, void*) {}
MirScreencast* create_screencast(MirScreencastSpec* spec)
{
    auto& server = spec->connection->display_server();
    auto screencast = std::make_unique<MirScreencast>(*spec, server, null_callback, nullptr);
    screencast->creation_wait_handle()->wait_for_all();

    auto raw_screencast = screencast.get();
    screencast.release();
    return raw_screencast;
}
}

MirScreencastSpec* mir_create_screencast_spec(MirConnection* connection)
{
    mir::require(mir_connection_is_valid(connection));
    return new MirScreencastSpec{connection};
}

void mir_screencast_spec_set_width(MirScreencastSpec* spec, unsigned width)
{
    spec->width = width;
}

void mir_screencast_spec_set_height(MirScreencastSpec* spec, unsigned height)
{
    spec->height = height;
}

void mir_screencast_spec_set_pixel_format(MirScreencastSpec* spec, MirPixelFormat format)
{
    spec->pixel_format = format;
}

void mir_screencast_spec_set_capture_region(MirScreencastSpec* spec, MirRectangle const* region)
{
    spec->capture_region = *region;
}

void mir_screencast_spec_release(MirScreencastSpec* spec)
{
    delete spec;
}

MirScreencast* mir_screencast_create_sync(MirScreencastSpec* spec)
try
{
    return create_screencast(spec);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return new MirScreencast(ex.what());
}

bool mir_screencast_is_valid(MirScreencast *screencast)
{
    return screencast && screencast->valid();
}

char const *mir_screencast_get_error_message(MirScreencast *screencast)
{
    return screencast->get_error_message();
}


MirScreencast* mir_connection_create_screencast_sync(
    MirConnection* connection,
    MirScreencastParameters* parameters)
{
    mir::require(mir_connection_is_valid(connection));
    MirScreencastSpec spec{connection, *parameters};
    return mir_screencast_create_sync(&spec);
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
