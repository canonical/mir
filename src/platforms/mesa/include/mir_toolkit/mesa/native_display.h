/*
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
 */

#ifndef MIR_TOOLKIT_MESA_NATIVE_DISPLAY_H
#define MIR_TOOLKIT_MESA_NATIVE_DISPLAY_H

#include <mir_toolkit/mir_native_buffer.h>
#include <mir_toolkit/client_types.h>

#define MIR_MESA_TRUE 1
#define MIR_MESA_FALSE 0

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C"
{
#endif

typedef struct MirMesaEGLNativeDisplay MirMesaEGLNativeDisplay;
typedef struct MirMesaEGLNativeSurface MirMesaEGLNativeSurface;

struct MirMesaEGLNativeDisplay
{
    int (*display_get_platform)(MirMesaEGLNativeDisplay* display,
                                MirPlatformPackage* package);
    void *context;
};

struct MirMesaEGLNativeSurface
{
    int (*surface_set_swapinterval)(MirMesaEGLNativeSurface* surface, int interval);
    int (*surface_advance_buffer)(MirMesaEGLNativeSurface* surface,
                                  MirBufferPackage* buffer_package);
    int (*surface_get_parameters)(MirMesaEGLNativeSurface* surface,
                                  MirSurfaceParameters* surface_parameters);
};

#ifdef __cplusplus
} // extern "C"
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MESA_NATIVE_DISPLAY_H */
