/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 * Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_GRAPHICS_X_GUEST_PLATFORM_H_
#define MIR_GRAPHICS_X_GUEST_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "display_helpers.h"

namespace mir
{
namespace graphics
{
namespace X
{

class GuestPlatform : public graphics::Platform
{
public:
    GuestPlatform(std::shared_ptr<NestedContext> const& /*nested_context*/);

    std::shared_ptr<GraphicBufferAllocator> create_buffer_allocator() override;
    std::shared_ptr<PlatformIpcOperations> make_ipc_operations() const override;
    
    std::shared_ptr<Display> create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
        std::shared_ptr<graphics::GLConfig> const&) override;
    EGLNativeDisplayType egl_native_display() const override;

private:
    std::shared_ptr<mir::udev::Context> udev;
    std::shared_ptr<graphics::mesa::helpers::DRMHelper> const drm;
    graphics::mesa::helpers::GBMHelper gbm;
};

}
}
}

#endif // MIR_GRAPHICS_X_GUEST_PLATFORM_H_
