/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "error_buffer.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mcl = mir::client;
namespace geom = mir::geometry;

mcl::ErrorBuffer::ErrorBuffer(
    std::string const& msg, int buffer_id,
    MirBufferCallback cb, void* context,
    MirConnection* connection) :
    error_msg(msg),
    buffer_id(buffer_id),
    cb(cb),
    cb_context(context),
    connection(connection)
{
}

bool mcl::ErrorBuffer::valid() const
{
    return false;
}

char const* mcl::ErrorBuffer::error_message() const
{
    return error_msg.c_str();
}

void mcl::ErrorBuffer::received()
{
    cb(reinterpret_cast<::MirBuffer*>(static_cast<mcl::MirBuffer*>(this)), cb_context);
}

int mcl::ErrorBuffer::rpc_id() const
{
    return buffer_id;
}

MirConnection* mcl::ErrorBuffer::allocating_connection() const
{
    return connection;
}

#define THROW_EXCEPTION \
{ \
    BOOST_THROW_EXCEPTION(std::logic_error("error: use of MirBuffer when mir_buffer_is_valid() is false"));\
}
void mcl::ErrorBuffer::submitted() THROW_EXCEPTION
void mcl::ErrorBuffer::received(MirBufferPackage const&) THROW_EXCEPTION
std::shared_ptr<mcl::ClientBuffer> mcl::ErrorBuffer::client_buffer() const THROW_EXCEPTION
MirGraphicsRegion mcl::ErrorBuffer::map_region() THROW_EXCEPTION
void mcl::ErrorBuffer::unmap_region() THROW_EXCEPTION

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirBufferUsage mcl::ErrorBuffer::buffer_usage() const THROW_EXCEPTION
#pragma GCC diagnostic pop

MirPixelFormat mcl::ErrorBuffer::pixel_format() const THROW_EXCEPTION
geom::Size mcl::ErrorBuffer::size() const THROW_EXCEPTION
void mcl::ErrorBuffer::increment_age() THROW_EXCEPTION
void mcl::ErrorBuffer::set_callback(MirBufferCallback, void*) THROW_EXCEPTION
