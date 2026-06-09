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

#ifndef MIRAL_LIVE_CONFIG_TEST_HELPERS_H
#define MIRAL_LIVE_CONFIG_TEST_HELPERS_H

#include <miral/live_config_overrides_list.h>

#include <filesystem>
#include <vector>

struct LoadRecord
{
    std::vector<std::filesystem::path> unchanged;
    std::vector<std::filesystem::path> fresh;
    std::vector<std::filesystem::path> modified;
    std::vector<std::filesystem::path> dropped;
};

inline auto record(miral::live_config::OverridesList const& list) -> LoadRecord
{
    LoadRecord rec;
    list.for_each(
        [&](std::filesystem::path const& p, auto&&) { rec.unchanged.push_back(p); },
        [&](std::filesystem::path const& p, auto&&) { rec.fresh.push_back(p); },
        [&](std::filesystem::path const& p, auto&&) { rec.modified.push_back(p); },
        [&](std::filesystem::path const& p) { rec.dropped.push_back(p); });
    return rec;
}

inline auto record_paths(miral::live_config::OverridesList const& list) -> std::vector<std::filesystem::path>
{
    std::vector<std::filesystem::path> all_paths;
    list.for_each(
        [&](std::filesystem::path const& p, auto&&) { all_paths.push_back(p); },
        [&](std::filesystem::path const& p, auto&&) { all_paths.push_back(p); },
        [&](std::filesystem::path const& p, auto&&) { all_paths.push_back(p); },
        [&](std::filesystem::path const& p) { all_paths.push_back(p); });
    return all_paths;
}

#endif
