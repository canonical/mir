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

#ifndef MIRAL_MRU_WINDOW_LIST_H
#define MIRAL_MRU_WINDOW_LIST_H

#include <miral/window.h>

#include <functional>
#include <vector>

namespace miral
{
class MRUWindowList
{
public:

    void push(Window const& window);
    void erase(Window const& window);
    auto top() const -> Window;

    using Enumerator = std::function<bool(Window& window)>;

    void enumerate(Enumerator const& enumerator) const;

private:
    std::vector<Window> windows;
};
}

#endif //MIRAL_MRU_WINDOW_LIST_H
