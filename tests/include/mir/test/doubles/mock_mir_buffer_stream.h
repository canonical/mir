/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_MIR_BUFFER_STREAM_H_
#define MIR_TEST_DOUBLES_MOCK_MIR_BUFFER_STREAM_H_

#include "src/include/client/mir/mir_buffer_stream.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockMirBufferStream : public MirBufferStream
{
    MOCK_METHOD2(release, MirWaitHandle*(MirBufferStreamCallback, void*));
    
    MOCK_CONST_METHOD0(get_parameters, MirWindowParameters());
    MOCK_METHOD0(get_current_buffer, std::shared_ptr<client::ClientBuffer>());
    MOCK_METHOD0(get_current_buffer_id, uint32_t());
    MOCK_METHOD0(egl_native_window, EGLNativeWindowType());
    MOCK_METHOD1(swap_buffers, MirWaitHandle*(std::function<void()> const&));
    MOCK_METHOD0(swap_buffers_sync, void());
    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<client::MemoryRegion>());
    MOCK_CONST_METHOD0(swap_interval, int());
    MOCK_METHOD1(set_swap_interval, MirWaitHandle*(int));
    MOCK_METHOD1(adopted_by, void(MirWindow*));
    MOCK_METHOD1(unadopted_by, void(MirWindow*));
    MOCK_METHOD0(platform_type, MirPlatformType(void));
    MOCK_METHOD0(get_current_buffer_package, MirNativeBuffer*(void));
    MOCK_METHOD0(get_create_wait_handle, MirWaitHandle*(void));
    MOCK_CONST_METHOD0(rpc_id, frontend::BufferStreamId(void));
    MOCK_CONST_METHOD0(valid, bool(void));
    MOCK_METHOD1(buffer_available, void(mir::protobuf::Buffer const&));
    MOCK_METHOD0(buffer_unavailable, void());
    MOCK_METHOD1(set_size, void(geometry::Size));
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_METHOD1(set_scale, MirWaitHandle*(float));
    MOCK_CONST_METHOD0(get_error_message, char const*(void));
    MOCK_CONST_METHOD0(connection, MirConnection*());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MOCK_CONST_METHOD0(render_surface, MirRenderSurface*());
#pragma GCC diagnostic pop
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_MIR_BUFFER_STREAM_H_
