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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "black_arrow.c"
#include "builtin_cursor_repository.h"

#include "mir/graphics/cursor_image.h"

#include <assert.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
struct BlackArrowCursorImage : public mg::CursorImage
{
    void const* raw_argb(geom::Size const& size)
    {
        // Builtin cursor does not support resizing atm.
        assert(size.width.as_uint32_t() == black_arrow.width);
        assert(size.height.as_uint32_t() == black_arrow.width);
        assert(black_arrow.bytes_per_pixel == 4);
        
        return black_arrow.pixel_data;
    }
};
}

std::shared_ptr<mg::CursorImage> mg::BuiltinCursorRepository::lookup_cursor(std::string const& /* theme_name */,
    std::string const& /* cursor_name */)
{
    // Builtin repository only has one cursor and theme.
    return std::make_shared<BlackArrowCursorImage>();
}
