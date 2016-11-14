/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "error_render_surface.h"

namespace mcl = mir::client;

mcl::ErrorRenderSurface::ErrorRenderSurface(
    std::string const& error_msg, MirConnection* conn) :
        error(error_msg),
        connection_(conn)
{
}

MirConnection* mcl::ErrorRenderSurface::connection() const
{
    return connection_;
}

mir::frontend::BufferStreamId mcl::ErrorRenderSurface::stream_id() const
{
    throw std::runtime_error(error);
}

mir::geometry::Size mcl::ErrorRenderSurface::size() const
{
    throw std::runtime_error(error);
}

void mcl::ErrorRenderSurface::set_size(mir::geometry::Size)
{
    throw std::runtime_error(error);
}

MirBufferStream* mcl::ErrorRenderSurface::get_buffer_stream(
    int /*width*/, int /*height*/,
    MirPixelFormat /*format*/,
    MirBufferUsage /*buffer_usage*/)
{
    throw std::runtime_error(error);
}

char const* mcl::ErrorRenderSurface::get_error_message() const
{
    return error.c_str();
}
