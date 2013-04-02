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
 */

#ifndef MIR_TOOLKIT_MESA_NATIVE_DISPLAY_H
#define MIR_TOOLKIT_MESA_NATIVE_DISPLAY_H

#include "mir_toolkit/c_types.h"

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C"
{
#endif

typedef struct MirMesaEGLNativeDisplay MirMesaEGLNativeDisplay;

struct MirMesaEGLNativeDisplay
{
    void (*display_get_platform)(MirMesaEGLNativeDisplay* display,
                                 MirPlatformPackage* package);
    void (*surface_get_current_buffer)(MirMesaEGLNativeDisplay* display,
                                       MirEGLNativeWindowType surface,
                                       MirBufferPackage* buffer_package);
    void (*surface_get_parameters)(MirMesaEGLNativeDisplay* display,
                                   MirEGLNativeWindowType surface,
                                   MirSurfaceParameters* surface_parameters);
    void (*surface_advance_buffer)(MirMesaEGLNativeDisplay* display,
                                   MirEGLNativeWindowType surface);

    MirConnection* context;
};

#ifdef __cplusplus
} // extern "C"
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MESA_NATIVE_DISPLAY_H */
