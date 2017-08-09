/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_CLIENT_SCREENCAST_STREAM_H
#define MIR_CLIENT_SCREENCAST_STREAM_H

#include "mir_wait_handle.h"
#include "mir/client/egl_native_surface.h"
#include "mir/client/client_buffer.h"
#include "mir/mir_buffer_stream.h"
#include "mir/geometry/size.h"

#include "mir_toolkit/client_types.h"

#include <EGL/eglplatform.h>

#include <queue>
#include <memory>
#include <mutex>

namespace google
{
namespace protobuf
{
class Closure;
}
}
namespace mir
{
namespace logging
{
class Logger;
}
namespace protobuf
{
class BufferStream;
class BufferStreamParameters;
class Void;
}
namespace client
{
namespace rpc
{
class DisplayServer;
}
class ClientBufferFactory;
class ClientBuffer;
class ClientPlatform;
class PerfReport;
struct MemoryRegion;
class ScreencastStream : public EGLNativeSurface, public MirBufferStream
{
public:
    ScreencastStream(
        MirConnection* connection,
        mir::client::rpc::DisplayServer& server,
        std::shared_ptr<ClientPlatform> const& native_window_factory,
        mir::protobuf::BufferStream const& protobuf_bs);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirWindowParameters get_parameters() const override;
#pragma GCC diagnostic pop
    MirWaitHandle* swap_buffers(std::function<void()> const& done) override;
    std::shared_ptr<mir::client::ClientBuffer> get_current_buffer() override;
    uint32_t get_current_buffer_id() override;
    int swap_interval() const override;
    MirWaitHandle* set_swap_interval(int interval) override;
    void adopted_by(MirWindow*) override;
    void unadopted_by(MirWindow*) override;
    std::chrono::microseconds microseconds_till_vblank() const override;
    void set_buffer_cache_size(unsigned int) override;

    EGLNativeWindowType egl_native_window() override;
    std::shared_ptr<MemoryRegion> secure_for_cpu_write() override;

    void swap_buffers_sync() override;
    void request_and_wait_for_configure(MirWindowAttrib attrib, int) override;
    MirNativeBuffer* get_current_buffer_package() override;
    MirPlatformType platform_type() override;

    frontend::BufferStreamId rpc_id() const override;
    bool valid() const override;

    void buffer_available(mir::protobuf::Buffer const& buffer) override;
    void buffer_unavailable() override;
    void set_size(geometry::Size) override;
    geometry::Size size() const override;
    MirWaitHandle* set_scale(float scale) override;
    char const* get_error_message() const override;
    MirConnection* connection() const override;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirRenderSurface* render_surface() const override;
#pragma GCC diagnostic pop

private:
    void process_buffer(protobuf::Buffer const& buffer);
    void process_buffer(protobuf::Buffer const& buffer, std::unique_lock<std::mutex>&);
    void screencast_buffer_received(std::function<void()> done);

    mutable std::mutex mutex;

    MirConnection* connection_;
    mir::client::rpc::DisplayServer& display_server;
    std::shared_ptr<ClientPlatform> const client_platform;
    std::shared_ptr<ClientBufferFactory> const factory;
    std::unique_ptr<mir::protobuf::BufferStream> protobuf_bs;
    int const swap_interval_{1};

    std::shared_ptr<void> egl_native_window_;

    MirWaitHandle create_wait_handle;
    MirWaitHandle release_wait_handle;
    MirWaitHandle screencast_wait_handle;
    MirWaitHandle interval_wait_handle;
    std::unique_ptr<mir::protobuf::Void> protobuf_void;

    std::shared_ptr<MemoryRegion> secured_region;

    geometry::Size buffer_size;
    std::string error_message;

    std::shared_ptr<ClientBuffer> current_buffer;
    int32_t current_buffer_id = -1;
};

}
}

#endif // MIR_CLIENT_SCREENCAST_STREAM_H
