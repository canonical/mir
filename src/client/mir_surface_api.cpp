/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_surface.h"
#include "mir_toolkit/mir_wait.h"
#include "mir_toolkit/mir_client_library_debug.h"

#include "mir_connection.h"
#include "mir_surface.h"
#include "error_connections.h"

namespace mcl = mir::client;

namespace
{

// assign_result is compatible with all 2-parameter callbacks
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}

}

MirWaitHandle* mir_connection_create_surface(
    MirConnection* connection,
    MirSurfaceParameters const* params,
    mir_surface_callback callback,
    void* context)
{
    if (mcl::ErrorConnections::instance().contains(connection)) return 0;

    try
    {
        return connection->create_surface(*params, callback, context);
    }
    catch (std::exception const&)
    {
        // TODO callback with an error surface
        return nullptr;
    }
}

MirSurface* mir_connection_create_surface_sync(
    MirConnection* connection,
    MirSurfaceParameters const* params)
{
    MirSurface* surface = nullptr;

    mir_wait_for(mir_connection_create_surface(connection, params,
        reinterpret_cast<mir_surface_callback>(assign_result),
        &surface));

    return surface;
}

void mir_surface_set_event_handler(MirSurface* surface,
                                   MirEventDelegate const* event_handler)
{
    surface->set_event_handler(event_handler);
}

MirEGLNativeWindowType mir_surface_get_egl_native_window(MirSurface* surface)
{
    return reinterpret_cast<MirEGLNativeWindowType>(surface->generate_native_window());
}

MirBool mir_surface_is_valid(MirSurface* surface)
{
    return MirSurface::is_valid(surface) ? mir_true : mir_false;
}

char const* mir_surface_get_error_message(MirSurface* surface)
{
    return surface->get_error_message();
}

void mir_surface_get_parameters(MirSurface* surface, MirSurfaceParameters* parameters)
{
    *parameters = surface->get_parameters();
}

MirPlatformType mir_surface_get_platform_type(MirSurface* surface)
{
    return surface->platform_type();
}

void mir_surface_get_current_buffer(MirSurface* surface, MirNativeBuffer** buffer_package_out)
{
    *buffer_package_out = surface->get_current_buffer_package();
}

void mir_surface_get_graphics_region(MirSurface* surface, MirGraphicsRegion* graphics_region)
{
    surface->get_cpu_region(*graphics_region);
}

MirWaitHandle* mir_surface_swap_buffers(
    MirSurface* surface,
    mir_surface_callback callback,
    void* context)
try
{
    return surface->next_buffer(callback, context);
}
catch (std::exception const&)
{
    return nullptr;
}

void mir_surface_swap_buffers_sync(MirSurface* surface)
{
    mir_wait_for(mir_surface_swap_buffers(surface,
        reinterpret_cast<mir_surface_callback>(assign_result),
        nullptr));
}

MirWaitHandle* mir_surface_release(
    MirSurface* surface,
    mir_surface_callback callback, void* context)
{
    try
    {
        return surface->release_surface(callback, context);
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

void mir_surface_release_sync(MirSurface* surface)
{
    mir_wait_for(mir_surface_release(surface,
        reinterpret_cast<mir_surface_callback>(assign_result),
        nullptr));
}

int mir_surface_get_id(MirSurface* surface)
{
    return mir_debug_surface_id(surface);
}

MirWaitHandle* mir_surface_set_type(MirSurface* surf,
                                    MirSurfaceType type)
{
    try
    {
        return surf ? surf->configure(mir_surface_attrib_type, type) : nullptr;
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

MirSurfaceType mir_surface_get_type(MirSurface* surf)
{
    MirSurfaceType type = mir_surface_type_normal;

    if (surf)
    {
        // Only the client will ever change the type of a surface so it is
        // safe to get the type from a local cache surf->attrib().

        int t = surf->attrib(mir_surface_attrib_type);
        type = static_cast<MirSurfaceType>(t);
    }

    return type;
}

MirWaitHandle* mir_surface_set_state(MirSurface* surf, MirSurfaceState state)
{
    try
    {
        return surf ? surf->configure(mir_surface_attrib_state, state) : nullptr;
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

MirSurfaceState mir_surface_get_state(MirSurface* surf)
{
    MirSurfaceState state = mir_surface_state_unknown;

    try
    {
        if (surf)
        {
            int s = surf->attrib(mir_surface_attrib_state);

            if (s == mir_surface_state_unknown)
            {
                surf->configure(mir_surface_attrib_state,
                                mir_surface_state_unknown)->wait_for_all();
                s = surf->attrib(mir_surface_attrib_state);
            }

            state = static_cast<MirSurfaceState>(s);
        }
    }
    catch (std::exception const&)
    {
    }

    return state;
}

MirOrientation mir_surface_get_orientation(MirSurface *surface)
{
    return surface->get_orientation();
}

MirWaitHandle* mir_surface_set_swapinterval(MirSurface* surf, int interval)
{
    if ((interval < 0) || (interval > 1))
        return nullptr;

    try
    {
        return surf ? surf->configure(mir_surface_attrib_swapinterval, interval) : nullptr;
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

int mir_surface_get_swapinterval(MirSurface* surf)
{
    int swap_interval = -1;

    try
    {
        swap_interval = surf ? surf->attrib(mir_surface_attrib_swapinterval) : -1;
    }
    catch (...)
    {
    }

    return swap_interval;
}

int mir_surface_get_dpi(MirSurface* surf)
{
    int dpi = -1;

    try
    {
        if (surf)
        {
            dpi = surf->attrib(mir_surface_attrib_dpi);
        }
    }
    catch (...)
    {
    }

    return dpi;
}

MirSurfaceFocusState mir_surface_get_focus(MirSurface* surf)
{
    MirSurfaceFocusState state = mir_surface_unfocused;

    try
    {
        if (surf)
        {
            state = static_cast<MirSurfaceFocusState>(surf->attrib(mir_surface_attrib_focus));
        }
    }
    catch (...)
    {
    }

    return state;
}

MirSurfaceVisibility mir_surface_get_visibility(MirSurface* surf)
{
    MirSurfaceVisibility state = static_cast<MirSurfaceVisibility>(mir_surface_visibility_occluded);

    try
    {
        if (surf)
        {
            state = static_cast<MirSurfaceVisibility>(surf->attrib(mir_surface_attrib_visibility));
        }
    }
    catch (...)
    {
    }

    return state;
}

MirWaitHandle* mir_surface_configure_cursor(MirSurface* surface, MirCursorConfiguration const* cursor)
{
    MirWaitHandle *result = nullptr;
    
    try
    {
        if (surface)
            result = surface->configure_cursor(cursor);
    }
    catch (...)
    {
    }

    return result;
}

/* Debug functions */

uint32_t mir_debug_surface_current_buffer_id(MirSurface* surface)
{
    return surface->get_current_buffer_id();
}

int mir_debug_surface_id(MirSurface* surface)
{
    return surface->id();
}
