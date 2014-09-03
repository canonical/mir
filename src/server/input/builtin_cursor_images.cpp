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
#include "builtin_cursor_images.h"

#include "mir/graphics/cursor_image.h"

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
struct BlackArrowCursorImage : public mg::CursorImage
{
    void const* as_argb_8888() const
    {
        return black_arrow.pixel_data;
    }
    geom::Size size() const
    {
        return { black_arrow.width, black_arrow.height };
    }
    geom::Displacement hotspot() const
    {
        return {0, 0};
    }
};
}

mi::BuiltinCursorImages::BuiltinCursorImages()
    : builtin_image(std::make_shared<BlackArrowCursorImage>())
{
}

std::shared_ptr<mg::CursorImage> mi::BuiltinCursorImages::image(std::string const& /* cursor_name */,
    geom::Size const& /* size */)
{
    // Builtin repository only has one cursor at a single size.
    return builtin_image;
}
