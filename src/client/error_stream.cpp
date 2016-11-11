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

#include "error_stream.h"

namespace mcl = mir::client;

mcl::ErrorBufferStream::ErrorBufferStream(
    MirRenderSurface* const rs,
    mir::client::rpc::DisplayServer& server,
    std::weak_ptr<SurfaceMap> const& map,
    std::string const& error_msg,
    MirConnection* conn,
    frontend::BufferStreamId id,
    std::shared_ptr<MirWaitHandle> const& wh) :
        BufferStream(server, map),
        rs(rs),
        error(error_msg),
        connection_(conn),
        id(id),
        wh(wh)
{
}

// Implement EGLNativeSurface API as NULL,
// as render_surface_buffer_stream_create_callback might call it
MirSurfaceParameters mcl::ErrorBufferStream::get_parameters() const
{
    return MirSurfaceParameters{"", 0, 0, mir_pixel_format_invalid, mir_buffer_usage_hardware, 0};
}

std::shared_ptr<mcl::ClientBuffer> mcl::ErrorBufferStream::get_current_buffer()
{
    return nullptr;
}

void mcl::ErrorBufferStream::request_and_wait_for_next_buffer() {}
void mcl::ErrorBufferStream::request_and_wait_for_configure(MirSurfaceAttrib /*a*/, int /*value*/) {}
void mcl::ErrorBufferStream::set_buffer_cache_size(unsigned int) {}

char const* mcl::ErrorBufferStream::get_error_message() const
{
    return error.c_str();
}

bool mcl::ErrorBufferStream::valid() const
{
    return false;
}

MirConnection* mcl::ErrorBufferStream::connection() const
{
    return connection_;
}

MirRenderSurface* mcl::ErrorBufferStream::render_surface() const
{
    return rs;
}

mir::frontend::BufferStreamId mcl::ErrorBufferStream::rpc_id() const
{
    return id;
}

uint32_t mcl::ErrorBufferStream::get_current_buffer_id()
{
    throw std::runtime_error(error);
}

EGLNativeWindowType mcl::ErrorBufferStream::egl_native_window()
{
    throw std::runtime_error(error);
}

MirWaitHandle* mcl::ErrorBufferStream::next_buffer(std::function<void()> const&)
{
    throw std::runtime_error(error);
}

std::shared_ptr<mcl::MemoryRegion> mcl::ErrorBufferStream::secure_for_cpu_write()
{
    throw std::runtime_error(error);
}

int mcl::ErrorBufferStream::swap_interval() const
{
    throw std::runtime_error(error);
}

MirWaitHandle* mcl::ErrorBufferStream::set_swap_interval(int)
{
    throw std::runtime_error(error);
}

MirNativeBuffer* mcl::ErrorBufferStream::get_current_buffer_package()
{
    throw std::runtime_error(error);
}

MirPlatformType mcl::ErrorBufferStream::platform_type()
{
    throw std::runtime_error(error);
}

MirWaitHandle* mcl::ErrorBufferStream::set_scale(float)
{
    throw std::runtime_error(error);
}

mir::geometry::Size mcl::ErrorBufferStream::size() const
{
    throw std::runtime_error(error);
}

void mcl::ErrorBufferStream::buffer_available(mir::protobuf::Buffer const&) {}
void mcl::ErrorBufferStream::buffer_unavailable() {}
void mcl::ErrorBufferStream::set_size(geometry::Size) {}

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

MirRenderSurface* mcl::ErrorStream::render_surface() const
{
    return nullptr;
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

mir::geometry::Size mcl::ErrorStream::size() const
{
    throw std::runtime_error(error);
}

void mcl::ErrorStream::buffer_available(mir::protobuf::Buffer const&) {}
void mcl::ErrorStream::buffer_unavailable() {}
void mcl::ErrorStream::set_size(mir::geometry::Size) {}
