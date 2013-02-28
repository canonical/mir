/*
 * Copyright © 2013 Canonical Ltd.
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir_client/shell.h"
#include "mir_surface.h"

using namespace mir;
using namespace mir::protobuf;

MirWaitHandle* mir_shell_surface_set_type(MirSurface *surf, MirSurfaceType type)
{
    return surf ? surf->modify(MIR_SURFACE_TYPE, type) : NULL;
}

MirSurfaceType mir_shell_surface_get_type(MirSurface *surf)
{
    MirSurfaceType type = MIR_SURFACE_NORMAL;

    if (surf)
    {
        // I assume the type can only change from the client side. Otherwise
        // we would have to send off a message to retrieve the latest...

        int t = surf->attrib(MIR_SURFACE_TYPE);
        type = static_cast<MirSurfaceType>(t);
    }

    return type;
}
