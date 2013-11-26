/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/buffer.h"
#include "mir/graphics/internal_surface.h"
#include "internal_native_surface.h"
#include <cstring>
namespace mgg = mir::graphics::gbm;

mgg::InternalNativeSurface::InternalNativeSurface(std::shared_ptr<InternalSurface> const& surface)
    : surface(surface)
{
    surface_advance_buffer = advance_buffer_static;
    surface_get_parameters = get_parameters_static;
    surface_set_swapinterval = set_swapinterval_static;
}

int mgg::InternalNativeSurface::advance_buffer_static(
    MirMesaEGLNativeSurface* surface, MirBufferPackage* package)
{
    auto native_surface = static_cast<mgg::InternalNativeSurface*>(surface);
    return native_surface->advance_buffer(package);
}

int mgg::InternalNativeSurface::advance_buffer(MirBufferPackage* package)
{
    surface->swap_buffers(current_buffer);

    auto buffer_package = current_buffer->native_buffer_handle();
    memcpy(package, buffer_package.get(), sizeof(MirBufferPackage));

    return MIR_MESA_TRUE;
}

int mgg::InternalNativeSurface::get_parameters_static(
    MirMesaEGLNativeSurface* surface, MirSurfaceParameters* parameters)
{
    auto native_surface = static_cast<mgg::InternalNativeSurface*>(surface);
    return native_surface->get_parameters(parameters);
}

int mgg::InternalNativeSurface::get_parameters(MirSurfaceParameters* parameters)
{
    auto size = surface->size();
    parameters->width = size.width.as_uint32_t();
    parameters->height = size.height.as_uint32_t();
    parameters->pixel_format = surface->pixel_format();
    parameters->buffer_usage = mir_buffer_usage_hardware;

    return MIR_MESA_TRUE;
}

int mgg::InternalNativeSurface::set_swapinterval_static(MirMesaEGLNativeSurface* surface, int interval)
{
    //TODO:
    (void) surface;
    (void) interval;
    return MIR_MESA_TRUE;
}
