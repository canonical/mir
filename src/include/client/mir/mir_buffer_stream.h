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

// MIR_MIR_? Yes.
//  The internal interface is mir/mir_buffer_stream.h
//  The public client API is  mir_toolkit/mir_buffer_stream.h
#ifndef MIR_MIR_BUFFER_STREAM_H_
#define MIR_MIR_BUFFER_STREAM_H_

#include "mir/frontend/buffer_stream_id.h"
#include "mir/geometry/size.h"

#include "mir_toolkit/client_types.h"
#include "mir_toolkit/mir_native_buffer.h"

#include <memory>
#include <functional>
#include <EGL/eglplatform.h>

/*
 * MirBufferStream::egl_native_window() returns EGLNativeWindowType.
 *
 * EGLNativeWindowType is an EGL platform-specific type that is typically a
 * (possibly slightly obfuscated) pointer. This makes our client module ABI
 * technically EGL-platform dependent, which is awkward because we support
 * multiple EGL platforms.
 *
 * On both the Mesa and the Android EGL platforms EGLNativeWindow is a
 * pointer or a uintptr_t.
 *
 * In practise EGLNativeWindowType is always a typedef to a pointer-ish, but
 * for paranoia's sake make sure the build will fail if we ever encounter a
 * strange EGL platform where this isn't the case.
 */
#include <type_traits>
static_assert(
    sizeof(EGLNativeWindowType) == sizeof(void*) &&
    std::is_pod<EGLNativeWindowType>::value,
    "The MirBufferStream requires that EGLNativeWindowType be no-op convertible to void*");

#undef EGLNativeWindowType
#define EGLNativeWindowType void*

class MirRenderSurface;
struct MirWaitHandle;

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
}
}

struct MirBufferStream
{
public:
    virtual ~MirBufferStream() = default;

    virtual MirWindowParameters get_parameters() const = 0;
    virtual std::shared_ptr<mir::client::ClientBuffer> get_current_buffer() = 0;
    virtual uint32_t get_current_buffer_id() = 0;
    virtual EGLNativeWindowType egl_native_window() = 0;
    virtual MirWaitHandle* swap_buffers(std::function<void()> const& done) = 0;
    virtual void swap_buffers_sync() = 0;

    virtual std::shared_ptr<mir::client::MemoryRegion> secure_for_cpu_write() = 0;

    virtual int swap_interval() const = 0;
    virtual MirWaitHandle* set_swap_interval(int interval) = 0;
    virtual void adopted_by(MirWindow*) = 0;
    virtual void unadopted_by(MirWindow*) = 0;

    virtual MirNativeBuffer* get_current_buffer_package() = 0;
    virtual MirPlatformType platform_type() = 0;

    virtual mir::frontend::BufferStreamId rpc_id() const = 0;
    
    virtual bool valid() const = 0;
    virtual void set_size(mir::geometry::Size) = 0;
    virtual mir::geometry::Size size() const = 0;
    virtual MirWaitHandle* set_scale(float) = 0;
    virtual char const* get_error_message() const = 0;
    virtual MirConnection* connection() const = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    virtual MirRenderSurface* render_surface() const = 0;
#pragma GCC diagnostic pop

    virtual void buffer_available(mir::protobuf::Buffer const& buffer) = 0;
    virtual void buffer_unavailable() = 0;
protected:
    MirBufferStream() = default;
    MirBufferStream(const MirBufferStream&) = delete;
    MirBufferStream& operator=(const MirBufferStream&) = delete;
};

#endif /* MIR_MIR_BUFFER_STREAM_H_ */
