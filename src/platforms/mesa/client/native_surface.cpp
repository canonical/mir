/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include <cstring>
#include "mir/client_buffer.h"
#include "native_surface.h"

#include <boost/exception/diagnostic_information.hpp> 

namespace mclm=mir::client::mesa;

namespace
{
static int advance_buffer_static(MirMesaEGLNativeSurface* surface,
                                  MirBufferPackage* buffer_package)
{
    auto s = static_cast<mclm::NativeSurface*>(surface);
    return s->advance_buffer(buffer_package);
}

static int get_parameters_static(MirMesaEGLNativeSurface* surface,
                                  MirSurfaceParameters* surface_parameters)
{
    auto s = static_cast<mclm::NativeSurface*>(surface);
    return s->get_parameters(surface_parameters);
}

static int set_swapinterval_static(MirMesaEGLNativeSurface* surface, int interval)
{
    auto s = static_cast<mclm::NativeSurface*>(surface);
    return s->set_swapinterval(interval);
}

void report_exception_at_driver_boundary(std::exception const& e)
{
    std::cerr << "Caught exception at Mir/EGL driver boundary: "
              << boost::diagnostic_information(e) << std::endl;
}
}

mclm::NativeSurface::NativeSurface(EGLNativeSurface& surface)
    : starting(true), surface(surface)
{
    surface_advance_buffer = advance_buffer_static;
    surface_get_parameters = get_parameters_static;
    surface_set_swapinterval = set_swapinterval_static;
}

int mclm::NativeSurface::advance_buffer(MirBufferPackage* buffer_package)
try
{
    /*
     * At present dri2_create_mir_window_surface will trigger
     * mir_advance_colour_buffer which will land here. Since we're still
     * creating the window, we don't have any buffers we want the server to
     * composite, so avoid sending a request to the server on startup:
     */
    if (starting)
        starting = false;
    else
        surface.request_and_wait_for_next_buffer();

    auto buffer = surface.get_current_buffer();

    auto buffer_to_driver = buffer->native_buffer_handle();
    memcpy(buffer_package, buffer_to_driver.get(), sizeof(MirBufferPackage));
    return MIR_MESA_TRUE;
}
catch (std::exception const& e)
{
    report_exception_at_driver_boundary(e);
    return MIR_MESA_FALSE;
}

int mclm::NativeSurface::get_parameters(MirSurfaceParameters* surface_parameters)
try
{
    auto params = surface.get_parameters();
    memcpy(surface_parameters, &params, sizeof(MirSurfaceParameters));
    return MIR_MESA_TRUE;
}
catch (std::exception const& e)
{
    report_exception_at_driver_boundary(e);
    return MIR_MESA_FALSE;
}

int mclm::NativeSurface::set_swapinterval(int interval)
try
{
    if ((interval < 0) || (interval > 1))
        return MIR_MESA_FALSE;

    surface.request_and_wait_for_configure(mir_surface_attrib_swapinterval, interval);
    return MIR_MESA_TRUE;
}
catch (std::exception const& e)
{
    report_exception_at_driver_boundary(e);
    return MIR_MESA_FALSE;
}
