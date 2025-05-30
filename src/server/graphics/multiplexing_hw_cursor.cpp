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

#include "multiplexing_hw_cursor.h"

#include "mir/graphics/display.h"
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;

namespace
{
auto construct_platform_cursors(std::span<mg::Display*> platform_displays) -> std::vector<std::shared_ptr<mg::Cursor>>
{
    std::vector<std::shared_ptr<mg::Cursor>> cursors;
    for (auto display : platform_displays)
    {
        cursors.push_back(display->create_hardware_cursor());
        if (cursors.back() == nullptr)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Platform failed to create hardware cursor"}));
        }
    }
    return cursors;
}
}

mg::MultiplexingCursor::MultiplexingCursor(std::span<Display*> platform_displays)
    : platform_cursors{construct_platform_cursors(platform_displays)}
{
}

void mg::MultiplexingCursor::show(std::shared_ptr<CursorImage> const& image)
{
    for (auto& cursor : platform_cursors)
    {
        cursor->show(image);
    }
}

void mg::MultiplexingCursor::hide()
{
    for (auto& cursor: platform_cursors)
    {
        cursor->hide();
    }
}

void mg::MultiplexingCursor::move_to(geometry::Point position)
{
    for (auto& cursor : platform_cursors)
    {
        cursor->move_to(position);
    }
}

void mir::graphics::MultiplexingCursor::scale(float new_scale)
{
    for(auto& cursor: platform_cursors)
        cursor->scale(new_scale);
}

auto mg::MultiplexingCursor::renderable() -> std::shared_ptr<Renderable>
{
    return nullptr;
}

