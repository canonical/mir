/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_MESA_NESTED_AUTHENTICATION_H_
#define MIR_GRAPHICS_MESA_NESTED_AUTHENTICATION_H_

#include "drm_authentication.h"

namespace mir
{
namespace graphics
{
class PlatformAuthentication;
class MesaAuthExtension;
namespace mesa
{
class NestedAuthentication : public DRMAuthentication
{
public:
    NestedAuthentication(std::shared_ptr<graphics::PlatformAuthentication> const& platform_authentication);
    void auth_magic(drm_magic_t magic) override;
    mir::Fd authenticated_fd() override;
private:
    std::shared_ptr<graphics::PlatformAuthentication> const platform_authentication;
    std::shared_ptr<MesaAuthExtension> const auth_extension;
};
}
}
}

#endif /* MIR_GRAPHICS_MESA_NESTED_AUTHENTICATION_H_ */
