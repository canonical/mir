/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mru_window_list.h"

#include <mir/scene/surface.h>

#include <algorithm>

namespace
{
bool visible(miral::Window const& window)
{
    std::shared_ptr<mir::scene::Surface> const& surface{window};

    switch (surface->state())
    {
    case mir_window_state_hidden:
    case mir_window_state_minimized:
        return false;
    default:
        return surface->visible();
    }
}
}

void miral::MRUWindowList::push(Window const& window)
{
    windows.erase(remove(begin(windows), end(windows), window), end(windows));
    windows.push_back(window);
}

void miral::MRUWindowList::erase(Window const& window)
{
    windows.erase(remove(begin(windows), end(windows), window), end(windows));
}

auto miral::MRUWindowList::top() const -> Window
{
    auto const& found = std::find_if(rbegin(windows), rend(windows), visible);
    return (found != rend(windows)) ? *found: Window{};
}

void miral::MRUWindowList::enumerate(Enumerator const& enumerator) const
{
    for (auto i = windows.rbegin(); i != windows.rend(); ++i)
        if (visible(*i))
            if (!enumerator(const_cast<Window&>(*i)))
                break;
}
