/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir_client/mir_client_library.h"
#include "mir_client/gbm/gbm_client_platform.h"
#include "mir_client/gbm/gbm_client_buffer_depository.h"

namespace mcl=mir::client;
namespace mclg=mir::client::gbm;
namespace geom=mir::geometry;

std::shared_ptr<mcl::ClientPlatform> mcl::create_client_platform(
        std::shared_ptr<MirPlatformPackage> const& /*platform_package*/)
{
    return std::make_shared<mclg::GBMClientPlatform>();
}

std::shared_ptr<mcl::ClientBufferDepository> mclg::GBMClientPlatform::create_platform_depository()
{
    return std::make_shared<mclg::GBMClientBufferDepository>();
}

EGLNativeWindowType mclg::GBMClientPlatform::create_egl_window(ClientSurface*)
{
    return (EGLNativeWindowType) -1;
}

void mclg::GBMClientPlatform::destroy_egl_window(EGLNativeWindowType)
{
}
