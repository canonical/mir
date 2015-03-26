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

#ifndef MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_STREAM_H_
#define MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_STREAM_H_

#include "src/client/client_buffer_stream.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct MockClientBufferStream : public client::ClientBufferStream
{
    MOCK_METHOD2(release, MirWaitHandle*(mir_buffer_stream_callback, void*));
    
    MOCK_CONST_METHOD0(get_parameters, MirSurfaceParameters());
    MOCK_METHOD0(get_current_buffer, std::shared_ptr<client::ClientBuffer>());
    MOCK_METHOD0(get_current_buffer_id, uint32_t());
    MOCK_METHOD0(egl_native_window, EGLNativeWindowType());
    MOCK_METHOD1(next_buffer, MirWaitHandle*(std::function<void()> const&));
    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<client::MemoryRegion>());
    MOCK_CONST_METHOD0(swap_interval, int());
    MOCK_METHOD1(set_swap_interval, void(int));
    MOCK_METHOD0(platform_type, MirPlatformType(void));
    MOCK_METHOD0(get_current_buffer_package, MirNativeBuffer*(void));
    MOCK_METHOD0(get_create_wait_handle, MirWaitHandle*(void));
    MOCK_CONST_METHOD0(rpc_id, frontend::BufferStreamId(void));
    MOCK_CONST_METHOD0(valid, bool(void));
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_STREAM_H_
