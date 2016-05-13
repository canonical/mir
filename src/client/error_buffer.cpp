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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "error_buffer.h"
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace geom = mir::geometry;

mcl::ErrorBuffer::ErrorBuffer(mir_buffer_callback cb, void* context, std::string msg) :
    cb(cb),
    cb_context(context),
    error_msg(msg)
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
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
void mcl::ErrorBuffer::submitted()
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
void mcl::ErrorBuffer::received(MirBufferPackage const&)
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
MirNativeBuffer* mcl::ErrorBuffer::as_mir_native_buffer() const
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
std::shared_ptr<mcl::ClientBuffer> mcl::ErrorBuffer::client_buffer() const
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
MirGraphicsRegion mcl::ErrorBuffer::map_region()
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
void mcl::ErrorBuffer::set_fence(MirNativeFence*, MirBufferAccess)
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
MirNativeFence* mcl::ErrorBuffer::get_fence() const
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
bool mcl::ErrorBuffer::wait_fence(MirBufferAccess, std::chrono::nanoseconds)
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
MirBufferUsage mcl::ErrorBuffer::buffer_usage() const
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
MirPixelFormat mcl::ErrorBuffer::pixel_format() const
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
geom::Size mcl::ErrorBuffer::size() const
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
MirConnection* mcl::ErrorBuffer::allocating_connection() const
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
void mcl::ErrorBuffer::increment_age()
{
    BOOST_THROW_EXCEPTION(std::logic_error("invalid buffer"));
}
