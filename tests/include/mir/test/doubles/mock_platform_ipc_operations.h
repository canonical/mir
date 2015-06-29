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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_PLATFORM_IPC_OPERATIONS_H_
#define MIR_TEST_DOUBLES_MOCK_PLATFORM_IPC_OPERATIONS_H_

#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/buffer_ipc_message.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockPlatformIpcOperations : public graphics::PlatformIpcOperations
{
    MockPlatformIpcOperations()
    {
        using namespace testing;
        ON_CALL(*this, connection_ipc_package())
            .WillByDefault(Return(std::make_shared<graphics::PlatformIPCPackage>()));
    }
    MOCK_CONST_METHOD3(pack_buffer,
        void(graphics::BufferIpcMessage&, graphics::Buffer const&, graphics::BufferIpcMsgType));
    MOCK_CONST_METHOD2(unpack_buffer,
        void(graphics::BufferIpcMessage&, graphics::Buffer const&));
    MOCK_METHOD0(connection_ipc_package, std::shared_ptr<graphics::PlatformIPCPackage>());
    MOCK_METHOD2(platform_operation, graphics::PlatformOperationMessage(
        unsigned int const, graphics::PlatformOperationMessage const&));
};
}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_PLATFORM_IPC_OPERATIONS_H_
