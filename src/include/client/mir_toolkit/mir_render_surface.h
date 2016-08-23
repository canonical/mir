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
 * Authored by:
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_TOOLKIT_MIR_RENDER_SURFACE_H_
#define MIR_TOOLKIT_MIR_RENDER_SURFACE_H_

#include <mir_toolkit/client_types_nbs.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

MirRenderSurface* mir_connection_create_render_surface(
    MirConnection* connection,
    int const width, int const height,
    MirPixelFormat const format);

bool mir_render_surface_is_valid(
    MirRenderSurface* render_surface);

void mir_render_surface_release(
    MirRenderSurface* render_surface);

void mir_surface_spec_add_render_surface(
    MirSurfaceSpec* spec,
    int scaled_width, int scaled_height,
    int displacement_x, int displacement_y,
    MirRenderSurface* render_surface);

#if 0
MirBufferStream* mir_render_surface_create_buffer_stream_sync(
    MirRenderSurface* render_surface,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage);

MirConnection* mir_render_surface_connection(
    MirRenderSurface* render_surface);

MirSurface* mir_render_surface_container(
    MirRenderSurface* render_surface);

MirEGLNativeWindowType mir_render_surface_egl_native_window(
    MirRenderSurface* render_surface);

MirPresentationChain* mir_render_surface_create_presentation_chain_sync(
    MirRenderSurface*);
#endif

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_RENDER_SURFACE_H_
