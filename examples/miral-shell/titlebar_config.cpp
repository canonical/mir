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

#include "titlebar_config.h"
#include <mutex>

namespace
{
std::mutex mutex;
std::string font_file{MIRAL_DEFAULT_FONT_FILE};
}

void titlebar::font_file(std::string const& font_file)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    ::font_file = font_file;
}

auto titlebar::font_file() -> std::string
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    return ::font_file;
}
