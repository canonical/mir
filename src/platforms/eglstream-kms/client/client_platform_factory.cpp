/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/client_platform_factory.h"
#include "client_platform.h"
#include "mir_toolkit/client_types.h"
#include "mir/client_context.h"
#include "mir/egl_native_display_container.h"
#include "mir/assert_module_entry_point.h"
#include "mir/module_deleter.h"

#include "mir_toolkit/mir_client_library.h"

#include <cstring>

namespace mcl = mir::client;
namespace mcle = mcl::eglstream;

namespace
{
bool is_eglstream_server(mcl::ClientContext* context)
{
    MirModuleProperties module_properties;

    context->populate_graphics_module(module_properties);

    return strncmp(module_properties.name, "mir:eglstream", strlen("mir:eglstream")) == 0;
}
}

mir::UniqueModulePtr<mcl::ClientPlatform> create_client_platform(mcl::ClientContext* context)
{
    mir::assert_entry_point_signature<mcl::CreateClientPlatform>(&create_client_platform);

    return mir::make_module_ptr<mcle::ClientPlatform>(context);
}

bool
is_appropriate_module(mcl::ClientContext* context)
{
    mir::assert_entry_point_signature<mcl::ClientPlatformProbe>(&is_appropriate_module);

    return is_eglstream_server(context);
}
