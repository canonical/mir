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

mcl::ErrorBuffer::ErrorBuffer(std::string const& msg, mir_buffer_callback cb, void* context) :
    error_msg(msg),
    cb(cb),
    cb_context(context)
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

#define THROW_EXCEPTION \
{ \
    BOOST_THROW_EXCEPTION(std::logic_error("error: use of MirBuffer when mir_buffer_is_valid() is false"));\
}
int mcl::ErrorBuffer::rpc_id() const THROW_EXCEPTION
void mcl::ErrorBuffer::submitted() THROW_EXCEPTION
void mcl::ErrorBuffer::received(MirBufferPackage const&) THROW_EXCEPTION
MirNativeBuffer* mcl::ErrorBuffer::as_mir_native_buffer() const THROW_EXCEPTION
std::shared_ptr<mcl::ClientBuffer> mcl::ErrorBuffer::client_buffer() const THROW_EXCEPTION
MirGraphicsRegion mcl::ErrorBuffer::map_region() THROW_EXCEPTION
void mcl::ErrorBuffer::set_fence(MirNativeFence*, MirBufferAccess) THROW_EXCEPTION
MirNativeFence* mcl::ErrorBuffer::get_fence() const THROW_EXCEPTION
bool mcl::ErrorBuffer::wait_fence(MirBufferAccess, std::chrono::nanoseconds) THROW_EXCEPTION
MirBufferUsage mcl::ErrorBuffer::buffer_usage() const THROW_EXCEPTION
MirPixelFormat mcl::ErrorBuffer::pixel_format() const THROW_EXCEPTION
geom::Size mcl::ErrorBuffer::size() const THROW_EXCEPTION
MirConnection* mcl::ErrorBuffer::allocating_connection() const THROW_EXCEPTION
void mcl::ErrorBuffer::increment_age() THROW_EXCEPTION
