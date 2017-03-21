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

#ifndef MIR_GRAPHICS_NULL_AUTHENTICATION_H_
#define MIR_GRAPHICS_NULL_AUTHENTICATION_H_
#include "mir/graphics/platform_authentication.h"

namespace mir
{
namespace graphics
{

class NullAuthentication : public graphics::PlatformAuthentication
{
    mir::optional_value<std::shared_ptr<graphics::MesaAuthExtension>> auth_extension() override;
    mir::optional_value<std::shared_ptr<graphics::SetGbmExtension>> set_gbm_extension() override;
    graphics::PlatformOperationMessage platform_operation(
        unsigned int, graphics::PlatformOperationMessage const&) override;
    mir::optional_value<mir::Fd> drm_fd() override;
};
}
}
#endif /* MIR_GRAPHICS_NULL_AUTHENTICATION_H_ */
