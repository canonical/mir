/*
 * Copyright © Canonical Ltd.
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
 */

#include "dynamic_default_cursor_image.h"
#include <mir/input/cursor_images.h>
#include <mir_toolkit/cursors.h>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mi::DynamicDefaultCursorImage::DynamicDefaultCursorImage(
    std::shared_ptr<CursorImages> const& cursor_images)
    : cursor_images{cursor_images},
      cached{cursor_images->image(mir_default_cursor_name, default_cursor_size)}
{
}

void const* mi::DynamicDefaultCursorImage::as_argb_8888() const
{
    auto locked = cached.lock();
    return *locked ? (*locked)->as_argb_8888() : nullptr;
}

geom::Size mi::DynamicDefaultCursorImage::size() const
{
    auto locked = cached.lock();
    return *locked ? (*locked)->size() : geom::Size{};
}

geom::Displacement mi::DynamicDefaultCursorImage::hotspot() const
{
    auto locked = cached.lock();
    return *locked ? (*locked)->hotspot() : geom::Displacement{};
}

void mi::DynamicDefaultCursorImage::refresh()
{
    auto locked = cached.lock();
    *locked = cursor_images->image(mir_default_cursor_name, default_cursor_size);
}
