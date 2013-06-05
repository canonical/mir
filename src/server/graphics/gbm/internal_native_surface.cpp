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

#include "internal_native_surface.h"
namespace mgg = mir::graphics::gbm;

mgg::InternalNativeSurface::InternalNativeSurface(std::shared_ptr<frontend::Surface> const& /*surface*/)
{
    surface_advance_buffer = advance_buffer_static;
    surface_get_parameters = get_parameters_static; 
}

void mgg::InternalNativeSurface::advance_buffer_static(MirMesaEGLNativeSurface* surface,
                                           MirBufferPackage* package)
{
    auto native_surface = static_cast<mgg::InternalNativeSurface*>(surface);
    native_surface->advance_buffer(package);
}
void mgg::InternalNativeSurface::advance_buffer(MirBufferPackage* /*package*/)
{
}

void mgg::InternalNativeSurface::get_parameters_static(MirMesaEGLNativeSurface* surface,
                                           MirSurfaceParameters* parameters)
{
    auto native_surface = static_cast<mgg::InternalNativeSurface*>(surface);
    native_surface->get_parameters(parameters);
}

void mgg::InternalNativeSurface::get_parameters(MirSurfaceParameters* /*parameters*/)
{
}
