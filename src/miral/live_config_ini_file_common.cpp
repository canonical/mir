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

#include "live_config_ini_file_common.h"

#include <mir/log.h>
#include <miral/live_config.h>

#include <string>

namespace mlc = miral::live_config;

namespace
{
auto trim_start_and_end(std::string_view string) -> std::string_view
{
    constexpr auto whitespace = " \t\n\r\f\v";

    auto const first_non_space = string.find_first_not_of(whitespace);
    if (first_non_space == std::string::npos)
    {
        return string.substr(0, 0);
    }

    auto const last_non_space = string.find_last_not_of(whitespace);
    return string.substr(first_non_space, last_non_space - first_non_space + 1);
}
}

auto mlc::parse_ini(std::istream& istream, std::filesystem::path path, std::function<void(Key const&, std::string_view, std::filesystem::path const&)> const &update_key) -> void
{
    for (std::string line; std::getline(istream, line);)
    {
        auto const line_view = std::string_view{line};
        if (!line_view.starts_with('#') && line_view.contains("="))
            try
            {
                auto const eq = line_view.find_first_of("=");
                auto const key = Key{trim_start_and_end(line_view.substr(0, eq))};
                auto const value = trim_start_and_end(line_view.substr(eq + 1));

                update_key(key, value, path);
            }
            catch (std::exception const& e)
            {
                mir::log_warning("Error processing '%s': %s", path.c_str(), e.what());
            }
    }
}
