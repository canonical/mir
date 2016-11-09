/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_EXTENSIONS_ANDORID_EGL_H_
#define MIR_CLIENT_EXTENSIONS_ANDORID_EGL_H_

#define MIR_EXTENSION_ANDROID_EGL "817e4327-bdd7-495a-9d3c-b5ac7a8a831f"
#define MIR_EXTENSION_ANDROID_EGL_VERSION(maj, min) (((maj) << 16) | (min))
#define MIR_EXTENSION_VERSION(maj, min) (((maj) << 16) | (min))
#define MIR_EXTENSION_ANDROID_EGL_VERSION_0_1 MIR_EXTENSION_VERSION(0,1)

#include "mir_toolkit/mir_connection.h"
#include "mir_toolkit/mir_render_surface.h"
#include "mir_toolkit/mir_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ANativeWindow;
struct ANativeWindowBuffer;
typedef void* (*mir_extension_to_native_display_type)(MirConnection*);
typedef ANativeWindow* (*mir_extension_create_anw)(MirBufferStream*);
typedef void (*mir_extension_destroy_anw)(ANativeWindow*);
typedef ANativeWindowBuffer* (*mir_extension_create_anwb)(MirBuffer*);
typedef void (*mir_extension_destroy_anwb)(ANativeWindowBuffer*);

struct MirExtensionAndroidEGL
{
    mir_extension_to_native_display_type to_display;
    mir_extension_create_anw create_window;
    mir_extension_destroy_anw destroy_window;
    mir_extension_create_anwb create_buffer;
    mir_extension_destroy_anwb destroy_buffer;
};

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_ANDORID_EGL_H_ */
