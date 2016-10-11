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
        mir::client::rpc::DisplayServer& server,
        std::weak_ptr<SurfaceMap> const& map,
        std::string const& error_msg,
        MirConnection* conn,
        frontend::BufferStreamId id,
        std::shared_ptr<MirWaitHandle> const& wh)
            : BufferStream(server, map),
              error(error_msg),
              connection_(conn),
              id(id),
              wh(wh)
{
}

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
    return nullptr;
}

mir::frontend::BufferStreamId mcl::ErrorBufferStream::rpc_id() const
{
    return id;
}

MirSurfaceParameters mcl::ErrorBufferStream::get_parameters() const
{
    throw std::runtime_error(error);
}

std::shared_ptr<mcl::ClientBuffer> mcl::ErrorBufferStream::get_current_buffer()
{
    throw std::runtime_error(error);
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

#if 0
mcl::ErrorStream::ErrorStream(
    std::string const& error_msg, RenderSurface* render_surface,
    mir::frontend::BufferStreamId id, std::shared_ptr<MirWaitHandle> const& wh) :
    error(error_msg),
    connection_(render_surface->connection()),
    id(id),
    wh(wh)
{
}
#endif

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

void mcl::ErrorStream::buffer_available(mir::protobuf::Buffer const&) {}
void mcl::ErrorStream::buffer_unavailable() {}
void mcl::ErrorStream::set_size(geometry::Size) {}
