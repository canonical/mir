/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CLIENT_BUFFER_STREAM_H
#define MIR_CLIENT_BUFFER_STREAM_H

#include "mir_wait_handle.h"
#include "mir/client/egl_native_surface.h"
#include "mir/client/client_buffer.h"
#include "mir/mir_buffer_stream.h"
#include "mir/geometry/size.h"
#include "mir/optional_value.h"
#include "buffer_stream_configuration.h"
#include "frame_clock.h"

#include "mir_toolkit/client_types.h"

#include <EGL/eglplatform.h>

#include <unordered_set>
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
class SurfaceMap;
class AsyncBufferFactory;
class ClientBufferFactory;
class ClientBuffer;
class ClientPlatform;
class PerfReport;
struct MemoryRegion;
class SurfaceMap;
class BufferDepository;
class BufferStream : public EGLNativeSurface, public MirBufferStream
{
public:
    BufferStream(
        mir::client::rpc::DisplayServer& server,
        std::weak_ptr<SurfaceMap> const& map);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    BufferStream(
        MirConnection* connection,
        MirRenderSurface* render_surface,
        std::shared_ptr<MirWaitHandle> creation_wait_handle,
        mir::client::rpc::DisplayServer& server,
        std::shared_ptr<ClientPlatform> const& native_window_factory,
        std::weak_ptr<SurfaceMap> const& map,
        std::shared_ptr<AsyncBufferFactory> const& factory,
        mir::protobuf::BufferStream const& protobuf_bs,
        std::shared_ptr<PerfReport> const& perf_report,
        std::string const& surface_name,
        geometry::Size ideal_size, size_t nbuffers);
#pragma GCC diagnostic pop

    virtual ~BufferStream();

    MirWaitHandle* swap_buffers(std::function<void()> const& done) override;
    std::shared_ptr<mir::client::ClientBuffer> get_current_buffer() override;
    // Required by debug API
    uint32_t get_current_buffer_id() override;

    int swap_interval() const override;
    MirWaitHandle* set_swap_interval(int interval) override;
    void adopted_by(MirWindow*) override;
    void unadopted_by(MirWindow*) override;
    void set_buffer_cache_size(unsigned int) override;

    EGLNativeWindowType egl_native_window() override;
    std::shared_ptr<MemoryRegion> secure_for_cpu_write() override;

    // mcl::EGLNativeSurface interface
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirWindowParameters get_parameters() const override;
#pragma GCC diagnostic pop
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

    std::chrono::microseconds microseconds_till_vblank() const override;

protected:
    BufferStream(BufferStream const&) = delete;
    BufferStream& operator=(BufferStream const&) = delete;

private:
    void process_buffer(protobuf::Buffer const& buffer);
    void process_buffer(protobuf::Buffer const& buffer, std::unique_lock<std::mutex>&);
    MirWaitHandle* set_server_swap_interval(int i);
    void wait_for_vsync();

    mutable std::mutex mutex; // Protects all members of *this

    MirConnection* connection_;
    std::shared_ptr<ClientPlatform> const client_platform;
    std::unique_ptr<mir::protobuf::BufferStream> protobuf_bs;

    optional_value<int> user_swap_interval;
    int current_swap_interval;
    bool using_client_side_vsync;
    BufferStreamConfiguration interval_config;
    float scale_;

    std::shared_ptr<mir::client::PerfReport> const perf_report;
    std::shared_ptr<void> egl_native_window_;

    std::unique_ptr<mir::protobuf::Void> protobuf_void;

    std::shared_ptr<MemoryRegion> secured_region;

    std::unique_ptr<BufferDepository> buffer_depository;
    geometry::Size ideal_buffer_size;
    size_t const nbuffers;
    std::string error_message;
    std::shared_ptr<MirWaitHandle> creation_wait_handle;
    std::weak_ptr<SurfaceMap> const map;
    std::shared_ptr<AsyncBufferFactory> const factory;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirRenderSurface* render_surface_;
#pragma GCC diagnostic pop

    std::unordered_set<MirWindow*> users;
    std::shared_ptr<FrameClock> frame_clock;
    mir::time::PosixTimestamp last_vsync;
    mutable mir::time::PosixTimestamp next_vsync;
};

}
}

#endif // MIR_CLIENT_BUFFER_STREAM_H
