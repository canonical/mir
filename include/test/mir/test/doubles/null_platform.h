/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include "mir/graphics/graphic_buffer_allocator.h"
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
    mir::UniqueModulePtr<graphics::GraphicBufferAllocator>
        create_buffer_allocator(graphics::Display const&) override
    {
        return nullptr;
    }

    mir::UniqueModulePtr<graphics::Display> create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
        std::shared_ptr<graphics::GLConfig> const&) override
    {
        return mir::make_module_ptr<NullDisplay>();
    }

    graphics::NativeDisplayPlatform* native_display_platform() override
    {
        return nullptr;
    }

    mir::UniqueModulePtr<graphics::PlatformIpcOperations> make_ipc_operations() const override
    {
        return mir::make_module_ptr<NullPlatformIpcOperations>();
    }

    graphics::NativeRenderingPlatform* native_rendering_platform() override
    {
        return nullptr;
    }

    std::vector<ExtensionDescription> extensions() const override
    {
        return {};
    }
};
}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_NULL_PLATFORM_
