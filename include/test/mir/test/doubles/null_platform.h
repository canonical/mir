/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_NULL_PLATFORM_H_
#define MIR_TEST_DOUBLES_NULL_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/null_platform_ipc_operations.h"

namespace mir
{
namespace test
{
namespace doubles
{
class NullPlatform : public graphics::Platform
{
 public:
    std::shared_ptr<graphics::GraphicBufferAllocator> create_buffer_allocator() override
    {
        return nullptr;
    }

    std::shared_ptr<graphics::Display> create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
        std::shared_ptr<graphics::GLProgramFactory> const&,
        std::shared_ptr<graphics::GLConfig> const&) override
    {
        return std::make_shared<NullDisplay>();
    }

    std::shared_ptr<graphics::PlatformIPCPackage> connection_ipc_package()
    {
        return std::make_shared<graphics::PlatformIPCPackage>();
    }

    std::shared_ptr<graphics::PlatformIpcOperations> make_ipc_operations() const override
    {
        return std::make_shared<NullPlatformIpcOperations>();
    }

    EGLNativeDisplayType egl_native_display() const override
    {
        return EGLNativeDisplayType();
    }
};
}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_NULL_PLATFORM_
