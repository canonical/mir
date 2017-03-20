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

#include "null_authentication.h"
#include "mir/graphics/platform_operation_message.h"

namespace mg = mir::graphics;

mir::optional_value<std::shared_ptr<mg::MesaAuthExtension>> mg::NullAuthentication::auth_extension()
{
    return {};
}
mir::optional_value<std::shared_ptr<mg::SetGbmExtension>> mg::NullAuthentication::set_gbm_extension()
{
    return {};
}

mg::PlatformOperationMessage mg::NullAuthentication::platform_operation(
    unsigned int, mg::PlatformOperationMessage const&)
{
    return mg::PlatformOperationMessage{};
}

mir::optional_value<mir::Fd> mg::NullAuthentication::drm_fd()
{
    return {};
}
