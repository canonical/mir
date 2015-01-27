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
 
#include "mir_protobuf.pb.h"

#include "mir_wait_handle.h"
#include "mir/egl_native_surface.h"
#include "mir/client_buffer.h"
#include "client_buffer_stream.h"
#include "client_buffer_depository.h"

#include "mir_toolkit/client_types.h"

#include <EGL/eglplatform.h>

#include <memory>
#include <mutex>

namespace mir
{
namespace logging
{
class Logger;
}
namespace client
{
class ClientBufferFactory;
class ClientBuffer;
class EGLNativeWindowFactory;
class PerfReport;
struct MemoryRegion;

enum BufferStreamMode
{
Producer, // As in surfaces
Consumer // As in screencasts
};

class BufferStream : public EGLNativeSurface, public ClientBufferStream
{
public:
    BufferStream(mir::protobuf::DisplayServer& server,
        BufferStreamMode mode,
        std::shared_ptr<ClientBufferFactory> const& buffer_factory,
        std::shared_ptr<EGLNativeWindowFactory> const& native_window_factory,
        protobuf::BufferStream const& protobuf_bs,
        std::shared_ptr<logging::Logger> const& logger);
    virtual ~BufferStream();
    
    MirWaitHandle* next_buffer(std::function<void()> const& done) override;
    std::shared_ptr<mir::client::ClientBuffer> get_current_buffer() override;
    // Required by debug API
    uint32_t get_current_buffer_id() override;
    
    int swap_interval() const override;
    void set_swap_interval(int interval) override;

    EGLNativeWindowType egl_native_window() override;
    std::shared_ptr<MemoryRegion> secure_for_cpu_write() override;

    // mcl::EGLNativeSurface interface
    MirSurfaceParameters get_parameters() const override;
    void request_and_wait_for_next_buffer() override;
    // TODO: In this context it seems like a wart that this is a "SurfaceAttribute"
    void request_and_wait_for_configure(MirSurfaceAttrib attrib, int) override;
    
protected:
    BufferStream(BufferStream const&) = delete;
    BufferStream& operator=(BufferStream const&) = delete;

private:
    void process_buffer(protobuf::Buffer const& buffer);
    void next_buffer_received(
        std::function<void()> done);
    void on_configured();
    void release_cpu_region();

    mutable std::mutex mutex; // Protects all members of *this

    mir::protobuf::DisplayServer& display_server;

    BufferStreamMode const mode;
    std::shared_ptr<EGLNativeWindowFactory> const native_window_factory;

    mir::protobuf::BufferStream protobuf_bs;
    mir::client::ClientBufferDepository buffer_depository;
    
    int swap_interval_;

    std::shared_ptr<mir::client::PerfReport> const perf_report;
    
    std::shared_ptr<EGLNativeWindowType> egl_native_window_;

    MirWaitHandle next_buffer_wait_handle;
    MirWaitHandle configure_wait_handle;
    
    std::shared_ptr<MemoryRegion> secured_region;
    
    geometry::Size cached_buffer_size;
};

}
}

#endif // MIR_CLIENT_BUFFER_STREAM_H
