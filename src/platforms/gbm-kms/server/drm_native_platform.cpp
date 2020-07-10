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

#include "drm_native_platform.h"
#include "display_helpers.h"
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgg = mg::gbm;

mgg::DRMNativePlatform::DRMNativePlatform(mgg::helpers::DRMHelper& drm) :
    drm(drm)
{
}

mir::optional_value<std::shared_ptr<mg::MesaAuthExtension>> mgg::DRMNativePlatform::auth_extension()
{
    class DRMAuth : public MesaAuthExtension
    {
    public:
        DRMAuth(mgg::helpers::DRMHelper& drm) :
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
                drm.auth_magic(magic);
                return 0;
            }
            catch ( std::runtime_error& ) 
            {
                return -1;
            }
        }
    private:
        mgg::helpers::DRMHelper& drm;
    };
    return {std::make_shared<DRMAuth>(drm)};
}

mir::optional_value<std::shared_ptr<mg::SetGbmExtension>> mgg::DRMNativePlatform::set_gbm_extension()
{
    return {};
}

mir::optional_value<mir::Fd> mgg::DRMNativePlatform::drm_fd()
{
    return {mir::Fd(IntOwnedFd{drm.fd})};
}

mgg::DRMNativePlatformAuthFactory::DRMNativePlatformAuthFactory(helpers::DRMHelper& drm) : drm(drm)
{
}

mir::UniqueModulePtr<mg::PlatformAuthentication> mgg::DRMNativePlatformAuthFactory::create_platform_authentication()
{
    return make_module_ptr<mgg::DRMNativePlatform>(drm);
}
