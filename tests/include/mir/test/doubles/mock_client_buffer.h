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

#ifndef MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_H_

#include "mir/client_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockClientBuffer : client::ClientBuffer
{
    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<client::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(stride, geometry::Stride());
    MOCK_CONST_METHOD0(pixel_format, MirPixelFormat());

    MOCK_CONST_METHOD0(age, uint32_t());
    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_METHOD0(increment_age, void());
    MOCK_METHOD1(update_from, void(MirBufferPackage const&));
    MOCK_METHOD1(fill_update_msg, void(MirBufferPackage&));
    MOCK_CONST_METHOD0(native_buffer_handle, std::shared_ptr<graphics::NativeBuffer>());
    MOCK_CONST_METHOD0(as_mir_native_buffer, MirNativeBuffer*());


    MOCK_METHOD2(set_fence, void(MirNativeFence*, MirBufferAccess));
    MOCK_CONST_METHOD0(get_fence, MirNativeFence*());
    MOCK_METHOD2(wait_fence, bool(MirBufferAccess, std::chrono::nanoseconds));
};
}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_FACTORY_H_
