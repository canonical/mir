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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CLIENT_CLIENT_BUFFER_STREAM_H_
#define MIR_CLIENT_CLIENT_BUFFER_STREAM_H_

#include "mir/frontend/buffer_stream_id.h"
#include "mir/geometry/size.h"

#include "mir_toolkit/client_types.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "mir_wait_handle.h"

#include <EGL/eglplatform.h>

#include <memory>
#include <functional>

namespace mir
{
namespace protobuf
{
class Buffer;
}
namespace client
{
class ClientBuffer;
class MemoryRegion;

class ClientBufferStream
{
public:
    virtual ~ClientBufferStream() = default;

    virtual MirWaitHandle* get_create_wait_handle() = 0;
    
    virtual MirSurfaceParameters get_parameters() const = 0;
    virtual std::shared_ptr<ClientBuffer> get_current_buffer() = 0;
    virtual uint32_t get_current_buffer_id() = 0;
    virtual EGLNativeWindowType egl_native_window() = 0;
    virtual MirWaitHandle* next_buffer(std::function<void()> const& done) = 0;

    virtual std::shared_ptr<MemoryRegion> secure_for_cpu_write() = 0;

    virtual int swap_interval() const = 0;
    virtual MirWaitHandle* set_swap_interval(int interval) = 0;

    virtual MirNativeBuffer* get_current_buffer_package() = 0;
    virtual MirPlatformType platform_type() = 0;

    virtual frontend::BufferStreamId rpc_id() const = 0;
    
    virtual MirWaitHandle* release(mir_buffer_stream_callback callback, void* context) = 0;

    virtual bool valid() const = 0;
    virtual void buffer_available(mir::protobuf::Buffer const& buffer) = 0;
    virtual void buffer_unavailable() = 0;
    virtual void set_size(geometry::Size) = 0;
    virtual MirWaitHandle* set_scale(float) = 0;
protected:
    ClientBufferStream() = default;
    ClientBufferStream(const ClientBufferStream&) = delete;
    ClientBufferStream& operator=(const ClientBufferStream&) = delete;
};

}
}

#endif /* MIR_CLIENT_CLIENT_BUFFER_STREAM_H_ */
