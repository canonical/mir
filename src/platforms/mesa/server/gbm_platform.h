/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include "buffer_allocator.h"

namespace mir
{
namespace graphics
{
namespace mesa
{
class GBMPlatform : public graphics::RenderingPlatform,
                    public graphics::NativeRenderingPlatform,
                    public renderer::gl::EGLPlatform
{
public:
    GBMPlatform(
        BypassOption option,
        BufferImportMethod import_method,
        std::shared_ptr<PlatformAuthentication> const& platform_authentication);
    GBMPlatform(
        BypassOption bypass_option,
        BufferImportMethod import_method,
        std::shared_ptr<mir::udev::Context> const& udev,
        std::shared_ptr<helpers::DRMHelper> const& drm);

    UniqueModulePtr<GraphicBufferAllocator>
        create_buffer_allocator(Display const& output) override;
    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;
    NativeRenderingPlatform* native_rendering_platform() override;
    MirServerEGLNativeDisplayType egl_native_display() const override;
private:
    BypassOption const bypass_option;
    BufferImportMethod const import_method;
    std::shared_ptr<graphics::PlatformAuthentication> const platform_authentication;
    std::shared_ptr<mir::udev::Context> udev;
    std::shared_ptr<graphics::mesa::helpers::DRMHelper> drm;
    std::shared_ptr<helpers::GBMHelper> const gbm;
    std::shared_ptr<DRMAuthentication> const auth;
};
}
}
}
#endif /* MIR_GRAPHICS_MESA_GBM_PLATFORM_H_ */
