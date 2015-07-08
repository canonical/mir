/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/client_platform_factory.h"
#include "mir_toolkit/client_types.h"
#include "mir/client_context.h"
#include "android_client_platform.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mcl = mir::client;
namespace mcla = mcl::android;

std::shared_ptr<mcl::ClientPlatform>
create_client_platform(mcl::ClientContext* context)
{
    MirPlatformPackage platform;
    context->populate_server_package(platform);
    if (platform.data_items != 0 || platform.fd_items != 0)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempted to create Android client platform on non-Android server"}));
    }
    return std::make_shared<mcla::AndroidClientPlatform>(context);
}

bool
is_appropriate_module(mcl::ClientContext* context)
{
    MirPlatformPackage platform;
    context->populate_server_package(platform);
    // TODO: Actually check what platform we're using, rather than blindly
    //       hope we can distinguish them from the stuff they've put in the
    //       PlatformPackage.
    return platform.data_items == 0 && platform.fd_items == 0;
}
