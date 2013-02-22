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

#include <mir_client/shell.h>

void mir_shell_surface_set_type(MirSurface *surf, MirSurfaceType type)
{
    /* TODO */
    (void)surf;
    (void)type;
}

MirSurfaceType mir_shell_surface_get_type(MirSurface *surf)
{
    (void)surf;
    return MIR_SURFACE_NORMAL;  /* TODO */
}
