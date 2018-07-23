/*
 * Copyright © 2016-2018 Canonical Ltd.
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
#include <unistd.h>
#include <mutex>

namespace
{
auto default_font() -> std::string
{
    for (std::string const file : { "Ubuntu-B.ttf", "FreeSansBold.ttf" })
    {
        for (auto const path : { "/usr/share/fonts/truetype/ubuntu-font-family/",   // Ubuntu < 18.04 Ubuntu-B.ttf
                                 "/usr/share/fonts/truetype/ubuntu/",               // Ubuntu >= 18.04 Ubuntu-B.ttf
                                 "/usr/share/fonts/truetype/freefont/",             // Debian FreeSansBold.ttf
                                 "/usr/share/fonts/gnu-free/",                      // Fedora FreeSansBold.ttf
                                 "/usr/share/fonts/TTF/"})                          // Arch Ubuntu-B.ttf
        {
            auto const full_path = path + file;
            if (access(full_path.c_str(), R_OK) == 0)
                return full_path;
        }
    }

    return "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf";
}

std::mutex mutex;
std::string font_file{default_font()};
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
