/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/geometry/pixel_format.h"
#include "mir_test/draw/patterns.h"

namespace mtd=mir::test::draw;
namespace geom=mir::geometry;

mtd::DrawPatternSolid::DrawPatternSolid(uint32_t color_value)
 : color_value(color_value)
{
}

void mtd::DrawPatternSolid::draw(MirGraphicsRegion const& region) const
{
    if (region.pixel_format != mir_pixel_format_abgr_8888 )
        throw(std::runtime_error("cannot draw region, incorrect format"));

    uint32_t *pixel = (uint32_t*) region.vaddr;
    int pixel_stride = region.stride / geom::bytes_per_pixel(geom::PixelFormat::abgr_8888);
    for(auto i = 0; i < region.height; i++)
    {
        for(auto j = 0; j < region.width; j++)
        {
            pixel[i * pixel_stride + j] = color_value;
        }
    }
}

bool mtd::DrawPatternSolid::check(MirGraphicsRegion const& region) const
{
    if (region.pixel_format != mir_pixel_format_abgr_8888 )
        throw(std::runtime_error("cannot check region, incorrect format"));

    uint32_t *pixel = (uint32_t*) region.vaddr;
    int pixel_stride = region.stride / geom::bytes_per_pixel(geom::PixelFormat::abgr_8888);
    for(auto i = 0; i< region.width; i++)
    {
        for(auto j = 0; j < region.height; j++)
        {
            if (pixel[j * pixel_stride + i] != color_value)
            {
                return false;
            }
        }
    }
    return true;
}
