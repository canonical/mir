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

#ifndef MIRAL_CONFIG_FILE_STORE_ADAPTER_H
#define MIRAL_CONFIG_FILE_STORE_ADAPTER_H

#include <miral/config_aggregator.h>

#include <filesystem>
#include <functional>
#include <istream>
#include <memory>
#include <set>
#include <span>

namespace miral
{

/// Connects a miral::ConfigFile (which provides streams of config files on change) to a
/// miral::live_config::ConfigAggregator. The base config file has the lowest priority;
/// override files (from the .d directory) are added in lexicographic order, with later
/// entries having higher priority.
struct ConfigFileStoreAdapter
{
    /// Factory that creates a complete Source for a given stream and path. Called on every reload.
    using SourceFactory =
        std::function<live_config::ConfigAggregator::Source(std::unique_ptr<std::istream>, std::filesystem::path const&)>;

    ConfigFileStoreAdapter(
        live_config::ConfigAggregator& aggregator,
        SourceFactory source_factory);

    ConfigFileStoreAdapter(ConfigFileStoreAdapter const&) = delete;
    ConfigFileStoreAdapter& operator=(ConfigFileStoreAdapter const&) = delete;
    ConfigFileStoreAdapter(ConfigFileStoreAdapter&&) = default;
    ConfigFileStoreAdapter& operator=(ConfigFileStoreAdapter&&) = delete;

    void operator()(std::span<std::pair<std::unique_ptr<std::istream>, std::filesystem::path>> arg);

private:
    live_config::ConfigAggregator& aggregator;
    SourceFactory source_factory;
    bool base_config_registered{false};
    std::set<std::filesystem::path> active_override_paths;
};

} // namespace miral

#endif // MIRAL_CONFIG_FILE_STORE_ADAPTER_H