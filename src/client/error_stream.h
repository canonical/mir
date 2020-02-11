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

#ifndef MIR_CLIENT_ERROR_STREAM_H_
#define MIR_CLIENT_ERROR_STREAM_H_

#include "mir/mir_buffer_stream.h"

#include <string>

namespace mir
{
namespace client
{
class ErrorStream : public MirBufferStream
{
public:
    ErrorStream(
        std::string const& error_msg, MirConnection* conn,
        frontend::BufferStreamId id, std::shared_ptr<MirWaitHandle> const& wh);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirWindowParameters get_parameters() const override;
#pragma GCC diagnostic pop
    std::shared_ptr<ClientBuffer> get_current_buffer() override;
    uint32_t get_current_buffer_id() override;
    EGLNativeWindowType egl_native_window() override;
    MirWaitHandle* swap_buffers(std::function<void()> const& done) override;
    void swap_buffers_sync() override;
    std::shared_ptr<MemoryRegion> secure_for_cpu_write() override;
    int swap_interval() const override;
    MirWaitHandle* set_swap_interval(int interval) override;
    void adopted_by(MirWindow*) override;
    void unadopted_by(MirWindow*) override;
    std::chrono::microseconds microseconds_till_vblank() const override;
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirRenderSurface* render_surface() const override;
#pragma GCC diagnostic pop

    MirWaitHandle* release(MirBufferStreamCallback callback, void* context);
private:
    std::string const error;
    MirConnection* const connection_;
    frontend::BufferStreamId id;
    std::shared_ptr<MirWaitHandle> const wh;
};
}
}
#endif
