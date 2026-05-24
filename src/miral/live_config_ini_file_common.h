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

#ifndef MIR_LIVE_CONFIG_INI_FILE_COMMON_H
#define MIR_LIVE_CONFIG_INI_FILE_COMMON_H

#include <filesystem>
#include <functional>

namespace miral::live_config
{
class Key;
auto parse_ini(
    std::istream& istream,
    std::filesystem::path path,
    std::function<void(Key const&, std::string_view, std::filesystem::path const&)> const& update_key) -> void;
}

#endif
