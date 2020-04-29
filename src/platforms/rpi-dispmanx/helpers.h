/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_RPI_DISPMANX_HELPERS_H_
#define MIR_RPI_DISPMANX_HELPERS_H_

#include "mir/geometry/dimensions.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"
#include "mir/geometry/dimensions.h"

#include "interface/vctypes/vc_image_types.h"
#include "interface/vmcs_host/vc_dispmanx_types.h"

namespace mir
{
namespace graphics
{
namespace rpi
{
auto vc_image_type_from_mir_pf(MirPixelFormat format) -> VC_IMAGE_TYPE_T;

auto dispmanx_resource_for(
    geometry::Size const& size,
    geometry::Stride stride,
    MirPixelFormat format)
    -> DISPMANX_RESOURCE_HANDLE_T;

auto dispmanx_resource_from_pixels(
    void const* pixels,
    geometry::Size const& size,
    geometry::Stride const& stride,
    MirPixelFormat format) -> DISPMANX_RESOURCE_HANDLE_T;
}
}
}

#endif //MIR_RPI_DISPMANX_HELPERS_H_
