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

#include "error_stream.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mcl = mir::client;

mcl::ErrorStream::ErrorStream(
    std::string const& error_msg, MirConnection* conn,
    mir::frontend::BufferStreamId id, std::shared_ptr<MirWaitHandle> const& wh) :
    error(error_msg),
    connection_(conn),
    id(id),
    wh(wh)
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirRenderSurface* mcl::ErrorStream::render_surface() const
{
    return nullptr;
}
#pragma GCC diagnostic pop

mir::frontend::BufferStreamId mcl::ErrorStream::rpc_id() const
{
    return id;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirWindowParameters mcl::ErrorStream::get_parameters() const
{
#pragma GCC diagnostic pop
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
std::shared_ptr<mcl::ClientBuffer> mcl::ErrorStream::get_current_buffer()
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
uint32_t mcl::ErrorStream::get_current_buffer_id()
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
EGLNativeWindowType mcl::ErrorStream::egl_native_window()
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
MirWaitHandle* mcl::ErrorStream::swap_buffers(std::function<void()> const&)
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
void mcl::ErrorStream::swap_buffers_sync()
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
std::shared_ptr<mcl::MemoryRegion> mcl::ErrorStream::secure_for_cpu_write()
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}

int mcl::ErrorStream::swap_interval() const
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
MirWaitHandle* mcl::ErrorStream::set_swap_interval(int)
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
void mcl::ErrorStream::adopted_by(MirWindow*)
{
}
void mcl::ErrorStream::unadopted_by(MirWindow*)
{
}

std::chrono::microseconds mcl::ErrorStream::microseconds_till_vblank() const
{
    return std::chrono::microseconds::zero();
}

MirNativeBuffer* mcl::ErrorStream::get_current_buffer_package()
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
MirPlatformType mcl::ErrorStream::platform_type()
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}
MirWaitHandle* mcl::ErrorStream::set_scale(float)
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}

mir::geometry::Size mcl::ErrorStream::size() const
{
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
}

void mcl::ErrorStream::buffer_available(mir::protobuf::Buffer const&) {}
void mcl::ErrorStream::buffer_unavailable() {}
void mcl::ErrorStream::set_size(mir::geometry::Size) {}
