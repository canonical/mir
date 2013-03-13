/*
 * Simple definitions common to client and server.
 *
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_COMMON_H_
#define MIR_COMMON_H_

/* This is C code. Not C++. */

/*
 * Attributes of a surface that the client and server/shell may wish to
 * get or set over the wire.
 */
typedef enum MirSurfaceAttrib
{
    mir_surface_attrib_type,
    mir_surface_attrib_arraysize_
} MirSurfaceAttrib;

typedef enum MirSurfaceType
{
    mir_surface_type_normal,
    mir_surface_type_utility,
    mir_surface_type_dialog,
    mir_surface_type_overlay,
    mir_surface_type_freestyle,
    mir_surface_type_popover,
    mir_surface_type_arraysize_
} MirSurfaceType;

/* TODO: Surface states here */

#endif
