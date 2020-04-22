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

#include "helpers.h"
#include "mir/geometry/size.h"
#include "mir/geometry/dimensions.h"

#include <interface/vmcs_host/vc_dispmanx.h>

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

auto mg::rpi::vc_image_type_from_mir_pf(MirPixelFormat format) -> VC_IMAGE_TYPE_T
{
    switch(format)
    {
    case mir_pixel_format_xbgr_8888:
        return VC_IMAGE_BGRX8888;
    case mir_pixel_format_argb_8888:
        return VC_IMAGE_ARGB8888;
    case mir_pixel_format_xrgb_8888:
        return VC_IMAGE_RGBX8888;
    case mir_pixel_format_rgb_565:
        return VC_IMAGE_RGB565;
    default:
        BOOST_THROW_EXCEPTION((std::runtime_error{"Unexpected pixel format"}));
    }
}

auto mg::rpi::dispmanx_resource_for(
    geom::Size const& size,
    geom::Stride stride,
    MirPixelFormat format)
    -> DISPMANX_RESOURCE_HANDLE_T
{
    uint32_t dummy;
    auto const handle = vc_dispmanx_resource_create(
        vc_image_type_from_mir_pf(format),
        size.width.as_uint32_t() | stride.as_uint32_t() << 16,
        size.height.as_uint32_t() | size.height.as_uint32_t() << 16,
        &dummy);
    if (handle == DISPMANX_NO_HANDLE)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to allocate DispmanX resource"}));
    }
    return handle;
}

auto mg::rpi::dispmanx_resource_from_pixels(
    void const* pixels,
    geom::Size const& size,
    geom::Stride const& stride,
    MirPixelFormat format) -> DISPMANX_RESOURCE_HANDLE_T
{
    uint32_t dummy;
    auto const vc_format = mg::rpi::vc_image_type_from_mir_pf(format);
    auto const width = size.width.as_uint32_t();
    auto const height = size.height.as_uint32_t();
    auto handle = vc_dispmanx_resource_create(vc_format, width, height, &dummy);

    VC_RECT_T rect;
    vc_dispmanx_rect_set(&rect, 0, 0, width, height);

    vc_dispmanx_resource_write_data(
        handle,
        vc_format,
        stride.as_uint32_t(),
        const_cast<unsigned char*>(static_cast<unsigned char const*>(pixels)),
        &rect);

    return handle;
}
