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
#include <mir_test_framework/temporary_environment_value.h>

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

struct XdgEnvGuard
{
    XdgEnvGuard(std::filesystem::path const& home, std::filesystem::path const& system) :
        home{"XDG_CONFIG_HOME", home.string().c_str()},
        dirs{"XDG_CONFIG_DIRS", system.string().c_str()}
    {}

    mir_test_framework::TemporaryEnvironmentValue const home;
    mir_test_framework::TemporaryEnvironmentValue const dirs;
};

#endif
