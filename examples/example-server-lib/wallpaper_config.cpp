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

#include "wallpaper_config.h"
#include <filesystem>
#include <mutex>
#include <vector>

namespace
{
/// Font search logic should be kept in sync with src/common/default_font.cpp
auto default_font() -> std::string
{
    struct FontPath
    {
        char const* filename;
        std::vector<char const*> prefixes;
    };

    FontPath const font_paths[]{
        FontPath{"Ubuntu-B.ttf", {
            "ubuntu-font-family",   // Ubuntu < 18.04
            "ubuntu",               // Ubuntu >= 18.04/Arch
        }},
        FontPath{"FreeSansBold.ttf", {
            "freefont",             // Debian/Ubuntu
            "gnu-free",             // Fedora/Arch
        }},
        FontPath{"DejaVuSans-Bold.ttf", {
            "dejavu",               // Ubuntu (others?)
            "",                     // Arch
        }},
        FontPath{"LiberationSans-Bold.ttf", {
            "liberation-sans",      // Fedora
            "liberation-sans-fonts",// Fedora >= 42
            "liberation",           // Arch/Ubuntu
        }},
        FontPath{"OpenSans-Bold.ttf", {
            "open-sans",            // Fedora/Ubuntu
        }},
    };

    char const* const font_path_search_paths[]{
        "/usr/share/fonts/truetype",    // Ubuntu/Debian
        "/usr/share/fonts/TTF",         // Arch
        "/usr/share/fonts",             // Fedora/Arch
    };

    std::vector<std::string> usable_search_paths;
    for (auto const& path : font_path_search_paths)
    {
        if (std::filesystem::exists(path))
            usable_search_paths.push_back(path);
    }

    for (auto const& font : font_paths)
    {
        for (auto const& prefix : font.prefixes)
        {
            for (auto const& path : usable_search_paths)
            {
                auto const full_font_path = path + '/' + prefix + '/' + font.filename;
                if (std::filesystem::exists(full_font_path))
                    return full_font_path;
            }
        }
    }

    return "";
}

std::mutex mutex;
std::string font_file{default_font()};
}

void wallpaper::font_file(std::string const& font_file)
{
    std::lock_guard lock{mutex};
    ::font_file = font_file;
}

auto wallpaper::font_file() -> std::string
{
    std::lock_guard lock{mutex};
    return ::font_file;
}
