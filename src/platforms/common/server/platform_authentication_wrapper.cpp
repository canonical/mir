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

#include "mir/graphics/platform_authentication_wrapper.h"
#include "mir/graphics/platform_operation_message.h"

namespace mg = mir::graphics;

mg::AuthenticationWrapper::AuthenticationWrapper(std::shared_ptr<PlatformAuthentication> const& auth)
    : auth(auth)
{
}

mir::optional_value<std::shared_ptr<mg::MesaAuthExtension>> mg::AuthenticationWrapper::auth_extension()
{
    return auth->auth_extension();
}

mir::optional_value<std::shared_ptr<mg::SetGbmExtension>> mg::AuthenticationWrapper::set_gbm_extension()
{
    return auth->set_gbm_extension();
}

mg::PlatformOperationMessage mg::AuthenticationWrapper::platform_operation(
    unsigned int op, mg::PlatformOperationMessage const& msg)
{
    return auth->platform_operation(op, msg);
}

mir::optional_value<mir::Fd> mg::AuthenticationWrapper::drm_fd()
{
    return auth->drm_fd();
}
