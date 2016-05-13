/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_MIR_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_MIR_BUFFER_H_

#include "src/client/buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockMirBuffer : client::Buffer
{
    MockMirBuffer() = default;
    MockMirBuffer(geometry::Size size, int id)
    {
        ON_CALL(*this, size()).WillByDefault(testing::Return(size));
        ON_CALL(*this, rpc_id()).WillByDefault(testing::Return(id));
    }
    MOCK_CONST_METHOD0(rpc_id, int());
    MOCK_METHOD0(submitted, void());
    MOCK_METHOD0(received, void());
    MOCK_METHOD1(received, void(MirBufferPackage const&));
    MOCK_CONST_METHOD0(as_mir_native_buffer, MirNativeBuffer*());
    MOCK_CONST_METHOD0(client_buffer, std::shared_ptr<client::ClientBuffer>());
    MOCK_METHOD0(map_region, MirGraphicsRegion());

    MOCK_METHOD2(set_fence, void(MirNativeFence*, MirBufferAccess));
    MOCK_CONST_METHOD0(get_fence, MirNativeFence*());
    MOCK_METHOD2(wait_fence, bool(MirBufferAccess, std::chrono::nanoseconds));

    MOCK_CONST_METHOD0(buffer_usage, MirBufferUsage());
    MOCK_CONST_METHOD0(pixel_format, MirPixelFormat());
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(allocating_connection, MirConnection*());
    MOCK_METHOD0(increment_age, void());
};

using StubMirBuffer = testing::NiceMock<MockMirBuffer>; 
}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_MIR_BUFFER_FACTORY_H_
