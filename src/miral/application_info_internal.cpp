/*
 * Copyright © 2016-2020 Canonical Ltd.
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

#include "application_info_internal.h"
#include "miral/window.h"

#include <mir/scene/session.h>

void miral::ApplicationInfo::add_window(Window const& window)
{
    self->windows.push_back(window);
}

void miral::ApplicationInfo::remove_window(Window const& window)
{
    auto& siblings = self->windows;

    for (auto i = begin(siblings); i != end(siblings); ++i)
    {
        if (window == *i)
        {
            siblings.erase(i);
            break;
        }
    }
}
