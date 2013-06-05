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
}

void mgg::InternalNativeSurface::native_display_surface_advance_buffer(MirEGLNativeWindowType /*surface*/,
                                           MirBufferPackage* /*package*/)
{
}

void mgg::InternalNativeSurface::native_display_surface_get_parameters(MirEGLNativeWindowType /*surface*/,
                                           MirSurfaceParameters* /*parameters*/)
{
}
