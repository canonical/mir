/*
 * Copyright © 2015 Canonical Ltd.
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

#include "error_stream.h"

namespace mcl = mir::client;

mcl::ErrorStream::ErrorStream(std::string const& error_msg, MirConnection* conn, mir::frontend::BufferStreamId id) :
    error(error_msg),
    connection_(conn),
    id(id)
{
}

char const* mcl::ErrorStream::get_error_message() const
{
    return error.c_str();
}

bool mcl::ErrorStream::valid() const
{
    return false;
}

MirConnection* mcl::ErrorStream::connection() const
{
    return connection_;
}

mir::frontend::BufferStreamId mcl::ErrorStream::rpc_id() const
{
    return id;
}

MirSurfaceParameters mcl::ErrorStream::get_parameters() const
{
    throw std::runtime_error(error);
}
std::shared_ptr<mcl::ClientBuffer> mcl::ErrorStream::get_current_buffer()
{
    throw std::runtime_error(error);
}
uint32_t mcl::ErrorStream::get_current_buffer_id()
{
    throw std::runtime_error(error);
}
EGLNativeWindowType mcl::ErrorStream::egl_native_window()
{
    throw std::runtime_error(error);
}
MirWaitHandle* mcl::ErrorStream::next_buffer(std::function<void()> const&)
{
    throw std::runtime_error(error);
}
std::shared_ptr<mcl::MemoryRegion> mcl::ErrorStream::secure_for_cpu_write()
{
    throw std::runtime_error(error);
}

int mcl::ErrorStream::swap_interval() const
{
    throw std::runtime_error(error);
}
MirWaitHandle* mcl::ErrorStream::set_swap_interval(int)
{
    throw std::runtime_error(error);
}
MirNativeBuffer* mcl::ErrorStream::get_current_buffer_package()
{
    throw std::runtime_error(error);
}
MirPlatformType mcl::ErrorStream::platform_type()
{
    throw std::runtime_error(error);
}
MirWaitHandle* mcl::ErrorStream::set_scale(float)
{
    throw std::runtime_error(error);
}

void mcl::ErrorStream::buffer_available(mir::protobuf::Buffer const&) {}
void mcl::ErrorStream::buffer_unavailable() {}
void mcl::ErrorStream::set_size(geometry::Size) {}
