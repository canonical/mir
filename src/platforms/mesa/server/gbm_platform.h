/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_GBM_PLATFORM_H_
#define MIR_GRAPHICS_MESA_GBM_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "display_helpers.h"
#include "platform_common.h"
#include "mir/renderer/gl/egl_platform.h"

namespace mir
{
namespace graphics
{
namespace mesa
{
class GBMPlatform : public graphics::RenderingPlatform,
                    public graphics::NativePlatform,
                    public renderer::gl::EGLPlatform
{
public:
    GBMPlatform(
        BypassOption option,
        std::shared_ptr<mir::graphics::NestedContext> const& nested_context);

    UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator() override;
    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;
    NativePlatform* native_platform() override;
    EGLNativeDisplayType egl_native_display() const override;
private:
    BypassOption bypass_option;
    std::shared_ptr<NestedContext> const nested_context;
    std::shared_ptr<helpers::GBMHelper> const gbm;
};
}
}
}
#endif /* MIR_GRAPHICS_MESA_GBM_PLATFORM_H_ */
