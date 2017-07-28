/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "builtin_cursor_images.h"

#include "mir/graphics/cursor_image.h"

#include "default-theme.h"

#include <algorithm>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
struct CursorImage : public mg::CursorImage
{
    CursorImage(CursorData const* const cursor_data) : cursor_data(cursor_data) {}

    void const* as_argb_8888() const
    {
        return cursor_data->pixel_data;
    }

    geom::Size size() const
    {
        return { cursor_data->width, cursor_data->height };
    }
    geom::Displacement hotspot() const
    {
        return {cursor_data->hotspot_x, cursor_data->hotspot_y};
    }

    CursorData const* const cursor_data;
};
}

std::shared_ptr<mg::CursorImage> mi::BuiltinCursorImages::image(std::string const& cursor_name,
    geom::Size const& /* size */)
{
    auto const match = std::find_if(begin(cursor_data), end(cursor_data), [&](CursorData const& data) { return cursor_name == data.name; });

    if (match != end(cursor_data))
        return std::make_shared<CursorImage>(&*match);

    return {};
}

