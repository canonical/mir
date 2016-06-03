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
#include "mir/assert_module_entry_point.h"
#include "android_client_platform.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cstring>

namespace mcl = mir::client;
namespace mcla = mcl::android;

mir::UniqueModulePtr<mcl::ClientPlatform>
create_client_platform(mcl::ClientContext* context)
{
    mir::assert_entry_point_signature<mcl::CreateClientPlatform>(&create_client_platform);
    MirPlatformPackage platform;
    context->populate_server_package(platform);
    if (platform.data_items != 0 || platform.fd_items != 0)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempted to create Android client platform on non-Android server"}));
    }
    return mir::make_module_ptr<mcla::AndroidClientPlatform>(context);
}

bool
is_appropriate_module(mcl::ClientContext* context)
{
    mir::assert_entry_point_signature<mcl::ClientPlatformProbe>(&is_appropriate_module);

    MirModuleProperties server_graphics_module;
    context->populate_graphics_module(server_graphics_module);

    return (strncmp("mir:android", server_graphics_module.name, strlen("mir:android")) == 0);
}
