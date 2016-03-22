/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
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
#include "mir/egl_native_surface.h"
#include "mir/client_buffer.h"
#include "client_buffer_stream.h"
#include "client_buffer_depository.h"
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
class SurfaceMap;
class ServerBufferSemantics;
class BufferStream : public EGLNativeSurface, public ClientBufferStream
{
public:
    BufferStream(
        MirConnection* connection,
        std::shared_ptr<MirWaitHandle> creation_wait_handle,
        mir::client::rpc::DisplayServer& server,
        std::shared_ptr<ClientPlatform> const& native_window_factory,
        mir::protobuf::BufferStream const& protobuf_bs,
        std::shared_ptr<PerfReport> const& perf_report,
        std::string const& surface_name,
        geometry::Size ideal_size, size_t nbuffers);
    // For surfaceless buffer streams
    BufferStream(
        MirConnection* connection,
        std::shared_ptr<MirWaitHandle> creation_wait_handle,
        mir::client::rpc::DisplayServer& server,
        std::shared_ptr<ClientPlatform> const& native_window_factory,
        mir::protobuf::BufferStreamParameters const& parameters,
        std::shared_ptr<PerfReport> const& perf_report,
        size_t nbuffers);

    virtual ~BufferStream();

    MirWaitHandle* next_buffer(std::function<void()> const& done) override;
    std::shared_ptr<mir::client::ClientBuffer> get_current_buffer() override;
    // Required by debug API
    uint32_t get_current_buffer_id() override;

    int swap_interval() const override;
    MirWaitHandle* set_swap_interval(int interval) override;
    void set_buffer_cache_size(unsigned int) override;

    EGLNativeWindowType egl_native_window() override;
    std::shared_ptr<MemoryRegion> secure_for_cpu_write() override;

    // mcl::EGLNativeSurface interface
    MirSurfaceParameters get_parameters() const override;
    void request_and_wait_for_next_buffer() override;

    void request_and_wait_for_configure(MirSurfaceAttrib attrib, int) override;

    MirNativeBuffer* get_current_buffer_package() override;

    MirPlatformType platform_type() override;

    frontend::BufferStreamId rpc_id() const override;
    bool valid() const override;

    void buffer_available(mir::protobuf::Buffer const& buffer) override;
    void buffer_unavailable() override;
    void set_size(geometry::Size) override;
    MirWaitHandle* set_scale(float scale) override;
    char const* get_error_message() const override;
    MirConnection* connection() const override;

protected:
    BufferStream(BufferStream const&) = delete;
    BufferStream& operator=(BufferStream const&) = delete;

private:
    void process_buffer(protobuf::Buffer const& buffer);
    void process_buffer(protobuf::Buffer const& buffer, std::unique_lock<std::mutex>&);
    void on_swap_interval_set(int interval);
    void on_scale_set(float scale);
    void release_cpu_region();
    MirWaitHandle* force_swap_interval(int interval);
    void init_swap_interval();

    mutable std::mutex mutex; // Protects all members of *this

    MirConnection* connection_;
    mir::client::rpc::DisplayServer& display_server;
    std::shared_ptr<ClientPlatform> const client_platform;
    std::unique_ptr<mir::protobuf::BufferStream> protobuf_bs;

    bool fixed_swap_interval;
    int swap_interval_;
    float scale_;

    std::shared_ptr<mir::client::PerfReport> const perf_report;
    std::shared_ptr<void> egl_native_window_;

    MirWaitHandle interval_wait_handle;
    std::unique_ptr<mir::protobuf::Void> protobuf_void;

    std::shared_ptr<MemoryRegion> secured_region;

    geometry::Size cached_buffer_size;

    std::unique_ptr<ServerBufferSemantics> buffer_depository;
    geometry::Size ideal_buffer_size;
    size_t const nbuffers;
    std::string error_message;
    std::shared_ptr<MirWaitHandle> creation_wait_handle;
    std::shared_ptr<SurfaceMap> const surface_map;
};

}
}

#endif // MIR_CLIENT_BUFFER_STREAM_H
