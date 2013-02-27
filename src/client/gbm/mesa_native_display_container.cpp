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

#include "mesa_native_display_container.h"

#include "mir_toolkit/mesa/native_display.h"
#include "mir_toolkit/api.h"

#include <unordered_set>
#include <mutex>

namespace mcl = mir::client;
namespace mclg = mcl::gbm;

namespace
{
mclg::MesaNativeDisplayContainer global_display_container;
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

}
}

mcl::EGLNativeDisplayContainer& mcl::EGLNativeDisplayContainer::instance()
{
    return global_display_container;
}

mclg::MesaNativeDisplayContainer::MesaNativeDisplayContainer()
{
}

bool
mclg::MesaNativeDisplayContainer::validate (MirEGLNativeDisplayType display) const
{
    std::lock_guard<std::mutex> lg(guard);
    return (valid_displays.find(display) != valid_displays.end());
}

MirEGLNativeDisplayType
mclg::MesaNativeDisplayContainer::create (MirConnection* connection)
{
    std::lock_guard<std::mutex> lg(guard);

    MirMesaEGLNativeDisplay* display = new MirMesaEGLNativeDisplay();
    display->display_get_platform = gbm_egl_display_get_platform;
    display->surface_get_current_buffer = gbm_egl_surface_get_current_buffer;
    display->surface_advance_buffer = gbm_egl_surface_advance_buffer;
    display->surface_get_parameters = gbm_egl_surface_get_parameters;
    display->context = connection;
    
    auto egl_display = reinterpret_cast<MirEGLNativeDisplayType>(display);
    valid_displays.insert(egl_display);
    
    return egl_display;
}

void
mclg::MesaNativeDisplayContainer::release (MirEGLNativeDisplayType display)
{
    std::lock_guard<std::mutex> lg(guard);
    
    auto it = valid_displays.find(display);
    if (it == valid_displays.end())
        return;

    delete reinterpret_cast<MirMesaEGLNativeDisplay*>(*it);
    valid_displays.erase(it);
}
    
