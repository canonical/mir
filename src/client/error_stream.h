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

#ifndef MIR_CLIENT_ERROR_STREAM_H_
#define MIR_CLIENT_ERROR_STREAM_H_

#include "client_buffer_stream.h"
#include "buffer_stream.h"

namespace mir
{
namespace client
{
class ErrorBufferStream : public BufferStream
{
public:
    ErrorBufferStream(
        MirRenderSurface* const rs,
        mir::client::rpc::DisplayServer& server,
        std::weak_ptr<SurfaceMap> const& map,
        std::string const& error_msg,
        MirConnection* conn,
        frontend::BufferStreamId id,
        std::shared_ptr<MirWaitHandle> const& wh);
    // EGLNativeSurface Interface
    MirSurfaceParameters get_parameters() const override;
    std::shared_ptr<ClientBuffer> get_current_buffer() override;
    void request_and_wait_for_next_buffer() override;
    void request_and_wait_for_configure(MirSurfaceAttrib a, int value) override;
    void set_buffer_cache_size(unsigned int) override;
    // ClientBufferStream Interface
    uint32_t get_current_buffer_id() override;
    EGLNativeWindowType egl_native_window() override;
    MirWaitHandle* next_buffer(std::function<void()> const& done) override;
    std::shared_ptr<MemoryRegion> secure_for_cpu_write() override;
    int swap_interval() const override;
    MirWaitHandle* set_swap_interval(int interval) override;
    MirNativeBuffer* get_current_buffer_package() override;
    MirPlatformType platform_type() override;
    frontend::BufferStreamId rpc_id() const override;
    bool valid() const override;
    void buffer_available(mir::protobuf::Buffer const& buffer) override;
    void buffer_unavailable() override;
    void set_size(geometry::Size) override;
    geometry::Size size() const override;
    MirWaitHandle* set_scale(float) override;
    char const* get_error_message() const override;
    MirConnection* connection() const override;
    MirRenderSurface* render_surface() const override;
    MirWaitHandle* release(mir_buffer_stream_callback callback, void* context);

private:
    MirRenderSurface* const rs;
    std::string const error;
    MirConnection* const connection_;
    frontend::BufferStreamId id;
    std::shared_ptr<MirWaitHandle> const wh;
};

class ErrorStream : public ClientBufferStream
{
public:
    ErrorStream(
        std::string const& error_msg, MirConnection* conn,
        frontend::BufferStreamId id, std::shared_ptr<MirWaitHandle> const& wh);

    MirSurfaceParameters get_parameters() const;
    std::shared_ptr<ClientBuffer> get_current_buffer();
    uint32_t get_current_buffer_id();
    EGLNativeWindowType egl_native_window();
    MirWaitHandle* next_buffer(std::function<void()> const& done);
    std::shared_ptr<MemoryRegion> secure_for_cpu_write();
    int swap_interval() const;
    MirWaitHandle* set_swap_interval(int interval);
    MirNativeBuffer* get_current_buffer_package();
    MirPlatformType platform_type();
    frontend::BufferStreamId rpc_id() const;
    MirWaitHandle* release(mir_buffer_stream_callback callback, void* context);
    bool valid() const;
    void buffer_available(mir::protobuf::Buffer const& buffer);
    void buffer_unavailable();
    void set_size(geometry::Size);
    geometry::Size size() const override;
    MirWaitHandle* set_scale(float);
    char const* get_error_message() const;
    MirConnection* connection() const;
    MirRenderSurface* render_surface() const;
private:
    std::string const error;
    MirConnection* const connection_;
    frontend::BufferStreamId id;
    std::shared_ptr<MirWaitHandle> const wh;
};
}
}
#endif
