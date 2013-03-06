/*
 * Surface-related declarations common to client and server
 * This is C. Not C++.
 *
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

#ifndef MIR_SURFACE_H_
#define MIR_SURFACE_H_

/*
 * Attributes of a surface that the client and server/shell may wish to
 * get or set over the wire.
 */
typedef enum MirSurfaceAttrib
{
    MIR_SURFACE_TYPE,
    MirSurfaceAttrib_ARRAYSIZE
} MirSurfaceAttrib;

typedef enum MirSurfaceType
{
    MIR_SURFACE_NORMAL,
    MIR_SURFACE_UTILITY,
    MIR_SURFACE_DIALOG,
    MIR_SURFACE_OVERLAY,
    MIR_SURFACE_FREESTYLE,
    MIR_SURFACE_POPOVER,
    MirSurfaceType_ARRAYSIZE
} MirSurfaceType;

/* TODO: Surface states here */

#endif
