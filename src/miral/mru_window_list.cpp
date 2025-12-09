/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mru_window_list.h"

#include <mir/scene/surface.h>

#include <algorithm>
#include <ranges>

namespace
{
bool visible(miral::Window const& window)
{
    std::shared_ptr<mir::scene::Surface> const& surface{window};
    return surface->state() != mir_window_state_hidden;
}
}

void miral::MRUWindowList::push(Window const& window)
{
    std::erase(windows, window);
    windows.push_back(window);
}

void miral::MRUWindowList::insert_below_top(Window const& window)
{
    std::erase(windows, window);
    if(!windows.empty())
        windows.insert(windows.end()-1, window);
    else
        windows.push_back(window);
}

void miral::MRUWindowList::erase(Window const& window)
{
    std::erase(windows, window);
}

auto miral::MRUWindowList::top() const -> Window
{
    auto const& found = std::find_if(rbegin(windows), rend(windows), visible);
    return (found != rend(windows)) ? *found: Window{};
}

void miral::MRUWindowList::enumerate(Enumerator const& enumerator) const
{
    for (auto const& window : std::ranges::reverse_view(windows))
        if (visible(window))
            if (!enumerator(const_cast<Window&>(window)))
                break;
}
