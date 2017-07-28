/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_EXTENSIONS_ANDROID_EGL_H_
#define MIR_CLIENT_EXTENSIONS_ANDROID_EGL_H_

#include "mir_toolkit/mir_connection.h"
#include "mir_toolkit/rs/mir_render_surface.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ANativeWindow;
struct ANativeWindowBuffer;
typedef void* (*mir_extension_to_native_display_type)(MirConnection*);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
typedef struct ANativeWindow* (*mir_extension_create_anw)(
    MirRenderSurface* rs,
    int width, int height,
    unsigned int hal_pixel_format,
    unsigned int gralloc_usage_flags);
#pragma GCC diagnostic pop
typedef void (*mir_extension_destroy_anw)(struct ANativeWindow*);
typedef struct ANativeWindowBuffer* (*mir_extension_create_anwb)(MirBuffer*);
typedef void (*mir_extension_destroy_anwb)(struct ANativeWindowBuffer*);

typedef struct MirExtensionAndroidEGLV1
{
    mir_extension_to_native_display_type to_display;
    mir_extension_create_anw create_window;
    mir_extension_destroy_anw destroy_window;
    mir_extension_create_anwb create_buffer;
    mir_extension_destroy_anwb destroy_buffer;
} MirExtensionAndroidEGLV1;

static inline MirExtensionAndroidEGLV1 const* mir_extension_android_egl_v1(
    MirConnection* connection)
{
    return (MirExtensionAndroidEGLV1 const*) mir_connection_request_extension(
        connection, "mir_extension_android_egl", 1);
}

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_ANDROID_EGL_H_ */
