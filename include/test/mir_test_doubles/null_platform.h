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
#include "null_display.h"

namespace mir
{
namespace test
{
namespace doubles
{
class NullPlatform : public graphics::Platform
{
 public:
    std::shared_ptr<graphics::GraphicBufferAllocator> create_buffer_allocator(
            const std::shared_ptr<graphics::BufferInitializer>& /*buffer_initializer*/)
    {
        return std::shared_ptr<graphics::GraphicBufferAllocator>();
    }

    std::shared_ptr<graphics::Display> create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&)
    {
        return std::make_shared<NullDisplay>();
    }

    std::shared_ptr<graphics::PlatformIPCPackage> get_ipc_package()
    {
        return std::make_shared<graphics::PlatformIPCPackage>();
    }

    std::shared_ptr<graphics::InternalClient> create_internal_client()
    {
        return std::shared_ptr<graphics::InternalClient>();
    }

    void fill_ipc_package(std::shared_ptr<graphics::BufferIPCPacker> const&,
                          std::shared_ptr<graphics::Buffer> const&) const
    {
    }

    EGLNativeDisplayType egl_native_display() const
    {
        return EGLNativeDisplayType();
    }
};
}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_NULL_PLATFORM_
