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

#ifndef MIR_GRAPHICS_PLATFORM_AUTHENTICATION_WRAPPER_H_
#define MIR_GRAPHICS_PLATFORM_AUTHENTICATION_WRAPPER_H_
#include "mir/graphics/platform_authentication.h"

namespace mir
{
namespace graphics
{
class AuthenticationWrapper : public PlatformAuthentication
{
public:
    AuthenticationWrapper(std::shared_ptr<PlatformAuthentication> const& auth);
    mir::optional_value<std::shared_ptr<MesaAuthExtension>> auth_extension() override;
    mir::optional_value<std::shared_ptr<SetGbmExtension>> set_gbm_extension() override;
    PlatformOperationMessage platform_operation(
        unsigned int op, PlatformOperationMessage const& msg) override;
    mir::optional_value<Fd> drm_fd() override;
private:
    std::shared_ptr<PlatformAuthentication> const auth;
};
}
}

#endif /* MIR_GRAPHICS_PLATFORM_AUTHENTICATION_WRAPPER_H_ */
