/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_NULL_PLATFORM_IPC_OPERATIONS_H_
#define MIR_TEST_DOUBLES_NULL_PLATFORM_IPC_OPERATIONS_H_

#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"

namespace mir
{
namespace test
{
namespace doubles
{
class NullPlatformIpcOperations : public graphics::PlatformIpcOperations
{
 public:
    void pack_buffer(graphics::BufferIpcMessage&, graphics::Buffer const&, graphics::BufferIpcMsgType) const override
    {
    }
    void unpack_buffer(graphics::BufferIpcMessage&, graphics::Buffer const&) const override
    {
    }
    std::shared_ptr<graphics::PlatformIPCPackage> connection_ipc_package() override
    {
        return std::make_shared<graphics::PlatformIPCPackage>();
    }
    graphics::PlatformOperationMessage platform_operation(
        unsigned int const, graphics::PlatformOperationMessage const&) override
    {
        return graphics::PlatformOperationMessage();
    }
};
}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_NULL_PLATFORM_IPC_OPERATIONS_H_
