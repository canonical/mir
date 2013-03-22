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
    mir_surface_attrib_state,
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

/*
 * MirSurfaceState: "minimized" or "minimised"?
 * A review of 7 leading dictionaries shows "z" is the most common spelling
 * (listed in all dictionaries), and "s" only appears in 4. And those that list
 * both "z" and "s" usually point out that "z" is the main spelling. Only 2
 * dictionaries say that "s" is preferred, and even still all dictionaries seem
 * to agree that "s" is only used in the UK.
 * The same is true for "maximized" vs "maximised".
 */
typedef enum MirSurfaceState
{
    mir_surface_state_unknown,
    mir_surface_state_restored,
    mir_surface_state_minimized,
    mir_surface_state_minimised = mir_surface_state_minimized,
    mir_surface_state_maximized,
    mir_surface_state_maximised = mir_surface_state_maximized,
    mir_surface_state_vertmaximized,
    mir_surface_state_vertmaximised = mir_surface_state_vertmaximized,
    /* mir_surface_state_semimaximized,
       Omitted for now, since it's functionally a subset of vertmaximized and
       differs only in the X coordinate. */
    mir_surface_state_fullscreen,
    mir_surface_state_arraysize_
} MirSurfaceState;

#endif
