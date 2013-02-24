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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "gbm_client_egl_native_display.h"
#include "mir_client/mir_client_library_mesa_egl.h"

#include <set>

namespace mclg = mir::client::gbm;

namespace
{
extern "C"
{

// TODO: This is a little nasty
std::set<MirMesaEGLNativeDisplay *> valid_native_displays;

static void gbm_egl_display_get_platform(MirMesaEGLNativeDisplay *display,
                                         MirPlatformPackage *package)
{
    MirConnection *mc = reinterpret_cast<MirConnection *>(display->context);
    mir_connection_get_platform (mc, package);
}

static void gbm_egl_surface_get_current_buffer (MirMesaEGLNativeDisplay *display,
                                                EGLNativeWindowType surface,
                                                MirBufferPackage *buffer_package)
{
    (void) display; // TODO:  Maybe we don't need this (or in other methods)
    MirSurface *ms = reinterpret_cast<MirSurface *>(surface);
    mir_surface_get_current_buffer(ms, buffer_package);
}

static void buffer_advanced_callback (MirSurface * /*surface*/,
                                      void * /*context*/)
{
}

static void gbm_egl_surface_advance_buffer (MirMesaEGLNativeDisplay *display,
                                            EGLNativeWindowType surface)
{
    (void) display; // TODO:  Maybe we don't need this (or in other methods)
    MirSurface *ms = reinterpret_cast<MirSurface *>(surface);
    mir_wait_for(mir_surface_next_buffer(ms, buffer_advanced_callback, nullptr));
}

static void gbm_egl_surface_get_parameters (MirMesaEGLNativeDisplay *display,
                                            EGLNativeWindowType surface,
                                            MirSurfaceParameters *surface_parameters)
{
    (void) display; // TODO:  Maybe we don't need this (or in other methods)
    MirSurface *ms = reinterpret_cast<MirSurface *>(surface);
    mir_surface_get_parameters(ms,  surface_parameters);
}

int mir_mesa_egl_native_display_is_valid(MirMesaEGLNativeDisplay *display)
{
    return (valid_native_displays.find(display) != valid_native_displays.end());
}

}
}


MirMesaEGLNativeDisplay *
mclg::EGL::create_native_display (MirConnection *connection)
{
    // TODO: Think about ownership here.
    MirMesaEGLNativeDisplay *display;
    
    display = new MirMesaEGLNativeDisplay();

    // TODO: Clean up names? This seems verbose
    display->display_get_platform = gbm_egl_display_get_platform;
    display->surface_get_current_buffer = gbm_egl_surface_get_current_buffer;
    display->surface_advance_buffer = gbm_egl_surface_advance_buffer;
    display->surface_get_parameters = gbm_egl_surface_get_parameters;
    
    display->context = (void *)connection;
    
    valid_native_displays.insert(display);
    
    return display;
}
    
