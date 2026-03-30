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

#include <mir/input/runtime_cursor_images.h>

namespace mi = mir::input;
namespace mg = mir::graphics;

mi::RuntimeCursorImages::RuntimeCursorImages(std::shared_ptr<CursorImages> inner)
    : state{State{std::move(inner), {}}}
{
}

std::shared_ptr<mg::CursorImage> mi::RuntimeCursorImages::image(
    std::string const& cursor_name,
    geometry::Size const& size)
{
    auto locked = state.lock();
    return locked->inner->image(cursor_name, size);
}

void mi::RuntimeCursorImages::set_cursor_images(std::shared_ptr<CursorImages> new_inner)
{
    std::vector<std::function<void()>> callbacks;
    {
        auto locked = state.lock();
        locked->inner = std::move(new_inner);
        callbacks = locked->change_callbacks;
    }

    for (auto const& cb : callbacks)
    {
        cb();
    }
}

void mi::RuntimeCursorImages::register_change_callback(std::function<void()> callback)
{
    auto locked = state.lock();
    locked->change_callbacks.push_back(std::move(callback));
}
