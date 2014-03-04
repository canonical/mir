/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir_toolkit/common.h"

template<size_t Rows, size_t Cols>
DrawPatternCheckered<Rows,Cols>::DrawPatternCheckered(uint32_t (&pattern) [Rows][Cols])
{
    for (size_t i=0; i<Rows; i++)
        std::memcpy(color_pattern[i], pattern[i], sizeof(uint32_t) * Cols);
}

template<size_t Rows, size_t Cols>
void DrawPatternCheckered<Rows,Cols>::draw(MirGraphicsRegion const& region) const
{
    if (region.pixel_format != mir_pixel_format_abgr_8888)
        throw(std::runtime_error("cannot draw region, incorrect format"));

    auto bpp = MIR_BYTES_PER_PIXEL(mir_pixel_format_abgr_8888);
    for(int i=0; i < region.width; i++)
    {
        for(int j=0; j<region.height; j++)
        {
            int key_row = i % Rows;
            int key_col = j % Cols;
            uint32_t *pixel = reinterpret_cast<uint32_t*>(&region.vaddr[j*region.stride + (i * bpp)]);
            *pixel = color_pattern[key_row][key_col];
        }
    }
}

template<size_t Rows, size_t Cols>
bool DrawPatternCheckered<Rows, Cols>::check(MirGraphicsRegion const& region) const
{
    if (region.pixel_format != mir_pixel_format_abgr_8888)
        throw(std::runtime_error("cannot check region, incorrect format"));

    auto bpp = MIR_BYTES_PER_PIXEL(mir_pixel_format_abgr_8888);
    for(int i=0; i< region.width; i++)
    {
        for(int j=0; j<region.height; j++)
        {
            int key_row = i % Rows;
            int key_col = j % Cols;
            uint32_t *pixel = reinterpret_cast<uint32_t*>(&region.vaddr[j*region.stride + (i * bpp)]);
            if ( *pixel != color_pattern[key_row][key_col] )
            {
                return false;
            }
        }
    }

    return true;
}
