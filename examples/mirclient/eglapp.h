/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef __EGLAPP_H__
#define __EGLAPP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <mir_toolkit/client_types.h>

typedef int mir_eglapp_bool;

struct mir_eglapp_arg
{
    char const* syntax;
    char const* format; /* "%" scanf format or "!"=flag, "$"=--, "="=copy */
    void* variable;
    char const* description;
};

extern float mir_eglapp_background_opacity;

mir_eglapp_bool mir_eglapp_init(int argc, char* argv[],
                                unsigned int* width, unsigned int* height,
                                struct mir_eglapp_arg const* custom_args);
void            mir_eglapp_swap_buffers(void);
void            mir_eglapp_quit(void);
mir_eglapp_bool mir_eglapp_running(void);
void            mir_eglapp_cleanup(void);
void            mir_eglapp_handle_event(MirWindow* window, MirEvent const* ev, void* unused);
void            egl_app_handle_resize_event(MirWindow* window, MirResizeEvent const* resize);
double          mir_eglapp_display_hz(void);

MirConnection* mir_eglapp_native_connection();
MirWindow*     mir_eglapp_native_window();
#ifdef __cplusplus
}
#endif

#endif
