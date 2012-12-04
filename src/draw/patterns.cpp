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

#include "mir/draw/patterns.h"

namespace md=mir::draw;

md::DrawPatternSolid::DrawPatternSolid(uint32_t color_value)
 : color_value(color_value)
{
}

void md::DrawPatternSolid::draw(std::shared_ptr<MirGraphicsRegion>& region) const
{
    // todo: should throw
    //if (region->pixel_format != mir_pixel_format_rgba_8888 )
    //    return false;

    uint32_t *pixel = (uint32_t*) region->vaddr;
    int i,j;
    for(i=0; i< region->width; i++)
    {
        for(j=0; j<region->height; j++)
        {
            pixel[j*region->width + i] = color_value;
        }
    }
}

bool md::DrawPatternSolid::check(const std::shared_ptr<MirGraphicsRegion>& region) const
{
    // todo: should throw
    //if (region->pixel_format != mir_pixel_format_rgba_8888 )
    //    return false;

    uint32_t *pixel = (uint32_t*) region->vaddr;
    int i,j;
    for(i=0; i< region->width; i++)
    {
        for(j=0; j<region->height; j++)
        {
            if (pixel[j*region->width + i] != color_value)
            {
                return false;
            }
        }
    }
    return true;
}
