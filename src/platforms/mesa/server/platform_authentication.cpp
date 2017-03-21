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

#include "platform_authentication.h"
#include "display_helpers.h"
#include "mir/graphics/platform_operation_message.h"
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;

mgm::PlatformAuthentication::PlatformAuthentication(mgm::helpers::DRMHelper& drm) :
    drm(drm)
{
}

mir::optional_value<std::shared_ptr<mg::MesaAuthExtension>> mgm::PlatformAuthentication::auth_extension()
{
    class DRMAuth : public MesaAuthExtension
    {
    public:
        DRMAuth(mgm::helpers::DRMHelper& drm) :
            drm(drm)
        {
        }

        mir::Fd auth_fd() override
        {
            return mir::Fd(drm.authenticated_fd());
        }

        int auth_magic(unsigned int magic) override
        {
            try
            {
                return auth_magic(magic);
            }
            catch ( std::runtime_error& ) 
            {
                return -1;
            }
        }
    private:
        mgm::helpers::DRMHelper& drm;
    };
    return {std::make_shared<DRMAuth>(drm)};
}

mir::optional_value<std::shared_ptr<mg::SetGbmExtension>> mgm::PlatformAuthentication::set_gbm_extension()
{
    return {};
}

mg::PlatformOperationMessage mgm::PlatformAuthentication::platform_operation(
    unsigned int, mg::PlatformOperationMessage const&)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("platform_operation deprecated"));
}

mir::optional_value<mir::Fd> mgm::PlatformAuthentication::drm_fd()
{
    return {mir::Fd(IntOwnedFd{drm.fd})};
}
