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

/**
 * \addtogroup mir_toolkit
 * @{
 */
/* This is C code. Not C++. */

/**
 * Attributes of a surface that the client and server/shell may wish to
 * get or set over the wire.
 */
typedef enum MirSurfaceAttrib
{
    mir_surface_attrib_type,
    mir_surface_attrib_state,
    mir_surface_attrib_swapinterval,
    mir_surface_attrib_focus,
    mir_surface_attribs
} MirSurfaceAttrib;

typedef enum MirSurfaceType
{
    mir_surface_type_normal,
    mir_surface_type_utility,
    mir_surface_type_dialog,
    mir_surface_type_overlay,
    mir_surface_type_freestyle,
    mir_surface_type_popover,
    mir_surface_type_inputmethod,
    mir_surface_types
} MirSurfaceType;

typedef enum MirSurfaceState
{
    mir_surface_state_unknown,
    mir_surface_state_restored,
    mir_surface_state_minimized,
    mir_surface_state_maximized,
    mir_surface_state_vertmaximized,
    /* mir_surface_state_semimaximized,
       Omitted for now, since it's functionally a subset of vertmaximized and
       differs only in the X coordinate. */
    mir_surface_state_fullscreen,
    mir_surface_states
} MirSurfaceState;

typedef enum MirSurfaceFocusState
{
    mir_surface_unfocused = 0,
    mir_surface_focused
} MirSurfaceFocusState;

typedef enum MirLifecycleState
{
    mir_lifecycle_state_will_suspend,
    mir_lifecycle_state_resumed,
    mir_lifecycle_connection_lost
} MirLifecycleState;

typedef enum MirPowerMode
{
    mir_power_mode_on, /* Display in use. */
    mir_power_mode_standby, /* Blanked, low power. */
    mir_power_mode_suspend, /* Blanked, lowest power. */
    mir_power_mode_off /* Powered down. */
} MirPowerMode;
/**@}*/

#endif
