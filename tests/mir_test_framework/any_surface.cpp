/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_test_framework/any_surface.h"

namespace mtf = mir_test_framework;

namespace 
{
    int width = 783;
    int height = 691;
    MirPixelFormat format = mir_pixel_format_abgr_8888;
}

MirSurface* mtf::make_any_surface(MirConnection *connection)
{
    auto spec = mir_connection_create_spec_for_normal_surface(connection,
        width, height, format);
    auto surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);
    
    return surface;
}
