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
#include "mir_toolkit/mesa/native_display.h"

#include <unordered_set>
#include <mutex>

namespace mclg = mir::client::gbm;

namespace
{
std::mutex native_display_guard;
std::unordered_set<MirMesaEGLNativeDisplay*> valid_native_displays;
}

namespace
{
extern "C"
{

static void gbm_egl_display_get_platform(MirMesaEGLNativeDisplay* display,
                                         MirPlatformPackage* package)
{
    mir_connection_get_platform(display->context, package);
}

static void gbm_egl_surface_get_current_buffer(MirMesaEGLNativeDisplay* /* display */,
                                                MirEGLNativeWindowType surface,
                                                MirBufferPackage* buffer_package)
{
    MirSurface* ms = reinterpret_cast<MirSurface*>(surface);
    mir_surface_get_current_buffer(ms, buffer_package);
}

static void buffer_advanced_callback(MirSurface*  /* surface */,
                                     void*  /* context */)
{
}

static void gbm_egl_surface_advance_buffer(MirMesaEGLNativeDisplay* /* display */,
                                           MirEGLNativeWindowType surface)
{
    MirSurface* ms = reinterpret_cast<MirSurface*>(surface);
    mir_wait_for(mir_surface_next_buffer(ms, buffer_advanced_callback, nullptr));
}

static void gbm_egl_surface_get_parameters(MirMesaEGLNativeDisplay* /* display */,
                                           MirEGLNativeWindowType surface,
                                           MirSurfaceParameters* surface_parameters)
{
    MirSurface* ms = reinterpret_cast<MirSurface*>(surface);
    mir_surface_get_parameters(ms,  surface_parameters);
}

int mir_egl_native_display_is_valid(MirEGLNativeDisplayType egl_display)
{
    std::lock_guard<std::mutex> lg(native_display_guard);

    auto display = reinterpret_cast<MirMesaEGLNativeDisplay*>(egl_display);
    return (valid_native_displays.find(display) != valid_native_displays.end());
}

}
}

MirMesaEGLNativeDisplay*
mclg::EGL::create_native_display (MirConnection* connection)
{
    std::lock_guard<std::mutex> lg(native_display_guard);
    MirMesaEGLNativeDisplay* display = new MirMesaEGLNativeDisplay();

    display->display_get_platform = gbm_egl_display_get_platform;
    display->surface_get_current_buffer = gbm_egl_surface_get_current_buffer;
    display->surface_advance_buffer = gbm_egl_surface_advance_buffer;
    display->surface_get_parameters = gbm_egl_surface_get_parameters;
    
    display->context = connection;
    
    valid_native_displays.insert(display);
    
    return display;
}

void
mclg::EGL::release_native_display (MirMesaEGLNativeDisplay* display)
{
    std::lock_guard<std::mutex> lg(native_display_guard);
    
    auto it = valid_native_displays.find(display);
    if (it == valid_native_displays.end())
        return;

    delete *it;
    valid_native_displays.erase(it);
}
    
