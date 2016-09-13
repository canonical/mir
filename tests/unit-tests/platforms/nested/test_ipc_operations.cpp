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

#include "src/server/graphics/nested/ipc_operations.h"
#include "src/server/graphics/nested/native_buffer.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/fake_shared.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/geometry/size.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct ForeignBuffer : mg::NativeBuffer
{
};

struct StubNestedBuffer : mgn::NativeBuffer
{
public:
    void sync(MirBufferAccess, std::chrono::nanoseconds) {}
    MirBuffer* client_handle() const { return nullptr; }
    MirNativeBuffer* get_native_handle() { return nullptr; }
    MirGraphicsRegion get_graphics_region()
    {
        return MirGraphicsRegion { 0, 0, 0, mir_pixel_format_invalid, nullptr };
    }
    geom::Size size() const { return {}; }
    MirPixelFormat format() const { return mir_pixel_format_invalid; }
};

struct MockIpcOperations : mg::PlatformIpcOperations
{
    MOCK_CONST_METHOD3(pack_buffer, void(mg::BufferIpcMessage&, mg::Buffer const&, mg::BufferIpcMsgType));
    MOCK_CONST_METHOD2(unpack_buffer, void(mg::BufferIpcMessage&, mg::Buffer const&));
    MOCK_METHOD0(connection_ipc_package, std::shared_ptr<mg::PlatformIPCPackage>());
    MOCK_METHOD2(platform_operation,
        mg::PlatformOperationMessage(unsigned int const, mg::PlatformOperationMessage const&));
};

struct NestedIPCOperations : testing::Test
{
    mtd::StubBuffer foreign_buffer{std::make_shared<ForeignBuffer>()};
    mtd::StubBuffer nested_buffer{std::make_shared<StubNestedBuffer>()};
    MockIpcOperations mock_ops;
};
}

TEST_F(NestedIPCOperations, uses_guest_platform_on_guest_buffers)
{
    mgn::IpcOperations operations(mt::fake_shared(mock_ops));
}
