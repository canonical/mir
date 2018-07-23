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

#ifndef MIR_GRAPHICS_MESA_DRM_NATIVE_PLATFORM_H_
#define MIR_GRAPHICS_MESA_DRM_NATIVE_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/graphics/platform_authentication.h"

namespace mir
{
namespace graphics
{
namespace mesa
{
namespace helpers
{
class DRMHelper;
}

class DRMNativePlatformAuthFactory : public graphics::NativeDisplayPlatform,
                                     public graphics::PlatformAuthenticationFactory
{
public:
    DRMNativePlatformAuthFactory(helpers::DRMHelper&);
    DRMNativePlatformAuthFactory(mir::Fd drm_fd);
    UniqueModulePtr<PlatformAuthentication> create_platform_authentication() override;
private:
    helpers::DRMHelper& drm;
};

class DRMNativePlatform : public graphics::PlatformAuthentication
{
public:
    DRMNativePlatform(helpers::DRMHelper&);
    mir::optional_value<std::shared_ptr<graphics::MesaAuthExtension>> auth_extension() override;
    mir::optional_value<std::shared_ptr<graphics::SetGbmExtension>> set_gbm_extension() override;
    PlatformOperationMessage platform_operation(
        unsigned int, PlatformOperationMessage const&) override;
    mir::optional_value<mir::Fd> drm_fd() override;
private:
    helpers::DRMHelper& drm;
};
}
}
}

#endif /* MIR_GRAPHICS_MESA_DRM_NATIVE_PLATFORM_H_ */
