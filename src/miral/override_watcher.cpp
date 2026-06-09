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

#include "override_watcher.h"

#include <mir/log.h>
#include <miral/live_config_overrides_list.h>
#include <miral/runner.h>

#include "live_config_overrides_list_builder.h"

#include <limits.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
namespace mlc = miral::live_config;
using path = fs::path;

namespace
{
auto watch_override_directory(
    mir::Fd const& inotify_fd, path const& config_root, std::string const& override_directory_name)
    -> mlc::InotifyWatch
{
    auto const override_directory_path = config_root / override_directory_name;
    std::error_code ec;

    if (!fs::is_directory(override_directory_path, ec))
    {
        auto const message = ec ? std::format(": {}", ec.message()) : "";
        mir::log_warning(
            "Override directory path '%s' does not point to a directory%s, ignoring",
            override_directory_path.c_str(),
            message.c_str());

        return {};
    }

    return {inotify_fd, override_directory_path};
}

auto classify_override_file(bool in_current, bool in_known, bool in_written) -> mlc::OverrideFileStatus
{
    using enum mlc::OverrideFileStatus;
    if (!in_current) return dropped;
    if (!in_known)   return fresh;
    return in_written ? modified : unchanged;
}

// Deduplicate events by basename, handling shadowing/unshadowing.
// A dropped file from a higher-priority root unshadows any lower-priority file with the same
// basename (promoting it to fresh). For all other statuses, the highest-priority event wins.
void merge_events_by_basename(
    std::map<std::string, mlc::OverrideFileEvent>& by_basename,
    std::vector<mlc::OverrideFileEvent>& root_events)
{
    for (auto& event : root_events)
    {
        auto const basename = event.path.filename().string();
        auto const status = event.status;

        if (status == mlc::OverrideFileStatus::dropped)
        {
            // If a lower-priority file with the same basename exists, promote it to fresh
            if (auto it = by_basename.find(basename);
                it != by_basename.end() && it->second.status == mlc::OverrideFileStatus::unchanged)
            {
                    it->second.status = mlc::OverrideFileStatus::fresh;
            }
            else
            {
                by_basename.insert_or_assign(basename, std::move(event));
            }
        }
        else
        {
            by_basename.insert_or_assign(basename, std::move(event));
        }
    }
}
}


mlc::OverrideWatcher::WatchedRoot::WatchedRoot(
    path dir,
    mlc::InotifyWatch base,
    mlc::InotifyWatch override_dir,
    std::set<path> known)
    : directory{std::move(dir)},
      base_dir_wd{std::move(base)},
      override_dir_wd{std::move(override_dir)},
      known_override_files{std::move(known)}
{
}

auto mlc::OverrideWatcher::WatchedRoot::has_base_watch() const -> bool
{
    return static_cast<bool>(base_dir_wd);
}

auto mlc::OverrideWatcher::WatchedRoot::classify_event(
    inotify_event const& event,
    path const& base_config_filename,
    path const& override_dir_name,
    std::string_view extension,
    BatchSummary& summary) const -> bool
{
    auto const event_name = event.len > 0 ? std::string_view{static_cast<char const*>(event.name)} : std::string_view{};
    auto const write_or_move = event.mask & (IN_CLOSE_WRITE | IN_MOVED_TO);
    auto const remove = event.mask & (IN_DELETE | IN_MOVED_FROM);

    auto const is_base_dir_event = base_dir_wd && event.wd == base_dir_wd.value();
    auto const is_override_dir_event = override_dir_wd && event.wd == override_dir_wd.value();

    if (write_or_move && is_base_dir_event && event_name == base_config_filename)
    {
        summary.base_config_changed = directory / base_config_filename;
        return true;
    }
    if (remove && is_base_dir_event && event_name == base_config_filename)
    {
        summary.base_config_removed = directory / base_config_filename;
        return true;
    }
    if ((event.mask & (IN_CREATE | IN_MOVED_TO)) && is_base_dir_event && event_name == override_dir_name)
    {
        summary.override_dir_events[directory] = OverrideDirEvent::created;
        return true;
    }
    if (remove && is_base_dir_event && event_name == override_dir_name)
    {
        summary.override_dir_events[directory] = OverrideDirEvent::removed;
        return true;
    }
    if ((write_or_move || remove) && is_override_dir_event && !event_name.empty()
        && event_name.ends_with(extension))
    {
        auto const override_dir_path = directory / override_dir_name;
        auto const changed_file_path = override_dir_path / path{event_name};
        auto const change_type = write_or_move ? FileChange::written : FileChange::removed;
        summary.override_file_changes[override_dir_path].push_back({changed_file_path, change_type});
        return true;
    }
    return false;
}

auto mlc::OverrideWatcher::WatchedRoot::on_directory_created(
    mir::Fd const& inotify_fd,
    std::string const& override_dir_name,
    std::string_view extension) -> std::vector<mlc::OverrideFileEvent>
{
    std::vector<OverrideFileEvent> events;

    // If the directory existed before, then this is likely an atomic replace.
    // Drop all previous overrides from this root.
    if (override_dir_wd)
    {
        for (auto const& f : known_override_files)
            events.push_back({f, OverrideFileStatus::dropped});
    }

    override_dir_wd = watch_override_directory(inotify_fd, directory, override_dir_name);
    known_override_files.clear();

    // Failed to watch for whatever reason.
    if (!override_dir_wd)
        return events;

    auto const override_dir_path = directory / override_dir_name;
    auto const current_overrides = mlc::collect_override_files(override_dir_path, extension);
    if (current_overrides.empty())
    {
        mir::log_debug(
            "Override directory '%s' created but contains no matching override files, skipping reload",
            override_dir_path.c_str());
        return events;
    }

    for (auto const& f : current_overrides)
        events.push_back({f, OverrideFileStatus::fresh});

    known_override_files = std::move(current_overrides);
    return events;
}

auto mlc::OverrideWatcher::WatchedRoot::on_directory_removed() -> std::vector<mlc::OverrideFileEvent>
{
    override_dir_wd.reset();
    std::vector<OverrideFileEvent> events;
    for (auto const& f : known_override_files)
        events.push_back({f, OverrideFileStatus::dropped});

    known_override_files.clear();
    return events;
}

auto mlc::OverrideWatcher::WatchedRoot::apply_file_changes(
    std::vector<BatchSummary::FileChangeEvent> const& root_changes,
    std::string const& override_dir_name,
    std::string_view extension) -> std::vector<mlc::OverrideFileEvent>
{
    auto const override_dir_path = directory / override_dir_name;
    auto const current_overrides = mlc::collect_override_files(override_dir_path, extension);

    std::set<path> written;
    for (auto const& [file, change] : root_changes)
        if (change == FileChange::written)
            written.insert(file);

    // Current files on disk + ones we knew about before
    std::set<path> all_files = current_overrides;
    all_files.insert(known_override_files.begin(), known_override_files.end());

    std::vector<OverrideFileEvent> events;
    for (auto const& f : all_files)
    {
        auto const classification = classify_override_file(
            current_overrides.contains(f), known_override_files.contains(f), written.contains(f));
        events.push_back({f, classification});
    }

    known_override_files = current_overrides;
    return events;
}

auto mlc::OverrideWatcher::WatchedRoot::collect_all_unchanged() const -> std::vector<mlc::OverrideFileEvent>
{
    std::vector<OverrideFileEvent> events;
    for (auto const& f : known_override_files)
        events.push_back({f, OverrideFileStatus::unchanged});
    return events;
}


mlc::OverrideWatcher::OverrideWatcher(path base_config, OverrideLoader load_config, std::string_view ext) :
    override_loader{std::move(load_config)},
    extension{std::move(ext)},
    base_config_filename{base_config.filename()},
    override_directory_name{base_config_filename.string() + ".d"}
{
    auto const config_roots = mlc::get_config_roots(base_config);

    // Build watched_roots in reverse priority order: lowest priority first
    for (auto const& dir : config_roots | std::views::reverse)
    {
        auto base_dir_wd = mlc::InotifyWatch{inotify_fd, dir};
        auto override_dir_wd = watch_override_directory(inotify_fd, dir, override_directory_name);
        auto known_override_files = override_dir_wd ?
                                          mlc::collect_override_files(dir / override_directory_name, extension) :
                                          std::set<path>{};
        watched_roots.emplace_back(dir, std::move(base_dir_wd), std::move(override_dir_wd), std::move(known_override_files));
    }
}

auto mlc::OverrideWatcher::should_register() const -> bool
{
    return std::ranges::any_of(watched_roots, [](auto const& r) { return r.has_base_watch(); });
}

// XDG config file lookup is first-found-wins. The highest-priority base config fully shadows
// lower-priority ones. Override (.d/) directories are merged across all roots.
auto mlc::OverrideWatcher::find_base_config() const -> std::optional<path>
{
    // Find base config in priority order (highest priority = last in watched_roots)
    for (auto const& root: watched_roots | std::views::reverse)
    {
        auto const candidate = root.directory / base_config_filename;
        std::error_code ec;
        if (fs::exists(candidate, ec))
            return candidate;
    }
    return std::nullopt;
}

auto mlc::OverrideWatcher::collect_inotify_events() const -> BatchSummary
{
    BatchSummary summary;

    for_each_inotify_event(
        [this, &summary](inotify_event const& event)
        {
            for (auto const& root : watched_roots)
                if (root.classify_event(event, base_config_filename, override_directory_name, extension, summary))
                    return;
        });

    return summary;
}

void mlc::OverrideWatcher::handler(int)
{
    // Phase 1: classify all events without mutating state or calling the loader
    auto const summary = collect_inotify_events();

    // Phase 2: act on the accumulated summary with at most one override_loader call
    if (auto maybe_overrides_list = apply_events(summary))
    {
        override_loader(*maybe_overrides_list);
        mir::log_debug("(Re)loaded [%s]", collect_paths(*maybe_overrides_list).c_str());
    }
}

auto mlc::OverrideWatcher::apply_events(BatchSummary const& summary) -> std::optional<mlc::OverridesList>
{
    if (summary.is_empty())
        return std::nullopt;

    try
    {
        auto const base_config = find_base_config();
        if (!base_config)
        {
            mir::log_debug("Base config '%s' not found in any config root", base_config_filename.c_str());
            return std::nullopt;
        }

        bool const active_base_config_changed =
            (summary.base_config_changed == base_config) || summary.base_config_removed;
        if (!active_base_config_changed && !summary.has_override_events())
            return std::nullopt;

        mlc::OverridesListBuilder builder;

        if (summary.base_config_changed == base_config)
            builder.push_modified(*base_config, mlc::open_file);
        else if (summary.base_config_removed)
        {
            builder.push_dropped(*summary.base_config_removed);
            builder.push_new(*base_config, mlc::open_file);
        }
        else
            builder.push_unchanged(*base_config, mlc::open_file);

        // Collect override file events from all roots (lowest priority first).
        // For files sharing a basename, only keep the highest-priority active file.
        // If a higher-priority file is dropped, any lower-priority file with the same
        // basename becomes visible (unshadowed).
        std::map<std::string, OverrideFileEvent> by_basename;
        bool override_state_changed = false;

        for (auto& root : watched_roots)
        {
            std::vector<OverrideFileEvent> root_events;

            // Check if this root has an override directory event.
            if (auto const dir_it = summary.override_dir_events.find(root.directory);
                dir_it != summary.override_dir_events.end())
            {
                if (dir_it->second == OverrideDirEvent::created)
                    root_events = root.on_directory_created(inotify_fd, override_directory_name, extension);
                else
                    root_events = root.on_directory_removed();
            }
            // What about file events?
            else if (auto const override_dir_path = root.directory / override_directory_name;
                     summary.override_file_changes.contains(override_dir_path))
            {
                auto const &root_changes = summary.override_file_changes.at(override_dir_path);
                root_events = root.apply_file_changes(root_changes, override_directory_name, extension);
            }
            else
            {
                root_events = root.collect_all_unchanged();
            }

            merge_events_by_basename(by_basename, root_events);
        }

        for (auto const& [_, event] : by_basename)
            if (event.status != OverrideFileStatus::unchanged)
                override_state_changed = true;

        if (!active_base_config_changed && !override_state_changed)
            return std::nullopt;

        // std::map is already sorted by key (basename), so iterate directly
        for (auto const& [_, event] : by_basename)
        {
            switch (event.status)
            {
            case OverrideFileStatus::unchanged: builder.push_unchanged(event.path, mlc::open_file); break;
            case OverrideFileStatus::fresh:     builder.push_new(event.path, mlc::open_file);       break;
            case OverrideFileStatus::modified:  builder.push_modified(event.path, mlc::open_file);  break;
            case OverrideFileStatus::dropped:   builder.push_dropped(event.path);                   break;
            }
        }

        return builder.build();
    }
    catch (std::exception const&)
    {
        mir::log(
            mir::logging::Severity::warning,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "Failed to reload configuration");
    }

    return std::nullopt;
}
