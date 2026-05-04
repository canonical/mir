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

#include <miral/config_file_store_adapter.h>

#include <map>
#include <algorithm>
#include <vector>

miral::ConfigFileStoreAdapter::ConfigFileStoreAdapter(
    live_config::ConfigAggregator& aggregator,
    SourceFactory source_factory)
    : aggregator{aggregator},
      source_factory{std::move(source_factory)}
{
}

void miral::ConfigFileStoreAdapter::operator()(
    std::span<std::pair<std::unique_ptr<std::istream>, std::filesystem::path>> arg)
{
    if (arg.empty())
        return;

    auto& [base_stream, base_config_path] = arg[0];
    auto const override_files = arg.subspan(1);

    if (!base_config_registered)
    {
        aggregator.add_source(source_factory(std::move(base_stream), base_config_path));
        base_config_registered = true;
    }
    else
    {
        aggregator.update_source(source_factory(std::move(base_stream), base_config_path));
    }

    // arg[1..] arrives lexicographically sorted. Compute the delta against the
    // previous invocation: remove gone sources, add new ones, and update retained
    // ones with fresh streams.
    std::vector<std::filesystem::path> new_override_paths;
    for (auto const& [_, path] : override_files)
        new_override_paths.push_back(path);

    // Derive previous paths from active_override_paths (a std::set, so already sorted)
    std::vector<std::filesystem::path> previous_override_paths{active_override_paths.begin(), active_override_paths.end()};

    std::vector<std::filesystem::path> removed, added;
    std::ranges::set_difference(previous_override_paths, new_override_paths, std::back_inserter(removed));
    std::ranges::set_difference(new_override_paths, previous_override_paths, std::back_inserter(added));

    for (auto const& path : removed)
    {
        aggregator.remove_source(path);
        active_override_paths.erase(path);
    }

    // Build a map from path to stream for quick lookup
    std::map<std::filesystem::path, std::unique_ptr<std::istream>*> stream_by_path;
    for (auto& [stream, path] : override_files)
        stream_by_path.emplace(path, &stream);

    for (auto const& path : added)
    {
        aggregator.add_source(source_factory(std::move(*stream_by_path.at(path)), path));
        active_override_paths.insert(path);
    }

    std::vector<std::filesystem::path> retained;
    std::ranges::set_difference(new_override_paths, added, std::back_inserter(retained));

    for (auto const& path : retained)
    {
        aggregator.update_source(source_factory(std::move(*stream_by_path.at(path)), path));
    }

    aggregator.reload_all();
}
