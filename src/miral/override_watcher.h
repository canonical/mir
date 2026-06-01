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

#ifndef MIRAL_OVERRIDE_WATCHER_H
#define MIRAL_OVERRIDE_WATCHER_H

#include <miral/config_file.h>

#include "live_config_watcher.h"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace miral::live_config { class OverridesListBuilder; class OverridesList; }
namespace mlc = miral::live_config;

namespace miral
{
namespace live_config
{

enum class OverrideFileStatus { dropped, unchanged, modified, fresh };

struct OverrideFileEvent
{
    std::filesystem::path path;
    OverrideFileStatus status;
};

class OverrideWatcher : public mlc::Watcher
{
public:
    using OverrideLoader = miral::ConfigFile::OverrideLoader;
    OverrideWatcher(std::filesystem::path base_config, OverrideLoader load_config, std::string_view extension);

    void handler(int) override;

protected:
    bool should_register() const override;

// Made public specifically for tests
public:
    enum class FileChange { written, removed };
    enum class OverrideDirEvent { created, removed };

    struct BatchSummary
    {
        using OverrideDirPath = std::filesystem::path;

        struct FileChangeEvent
        {
            std::filesystem::path file;
            FileChange change;
        };

        std::optional<std::filesystem::path> base_config_changed;
        std::optional<std::filesystem::path> base_config_removed;
        // Last event per root wins — inotify delivers events in order, so this naturally reflects net state
        std::map<std::filesystem::path, OverrideDirEvent> override_dir_events;
        std::map<OverrideDirPath, std::vector<FileChangeEvent>> override_file_changes;

        auto is_empty() const -> bool
        {
            return !base_config_changed && !base_config_removed && !has_override_events();
        }

        auto has_override_events() const -> bool
        {
            return !override_dir_events.empty() || !override_file_changes.empty();
        }
    };

    class WatchedRoot
    {
    public:
        WatchedRoot(
            std::filesystem::path directory,
            mlc::InotifyWatch base_dir_wd,
            mlc::InotifyWatch override_dir_wd,
            std::set<std::filesystem::path> known_override_files);

        /// Returns true if this root claims the event (populates summary), false otherwise.
        auto classify_event(
            inotify_event const& event,
            std::string_view base_config_filename,
            std::string_view override_dir_name,
            std::string_view extension,
            BatchSummary& summary) const -> bool;

        auto on_directory_created(
            mir::Fd const& inotify_fd,
            std::string const& override_dir_name,
            std::string_view extension) -> std::vector<OverrideFileEvent>;
        auto on_directory_removed() -> std::vector<OverrideFileEvent>;
        auto apply_file_changes(
            std::vector<BatchSummary::FileChangeEvent> const& changes,
            std::string const& override_dir_name,
            std::string_view extension) -> std::vector<OverrideFileEvent>;
        auto collect_all_unchanged() const -> std::vector<OverrideFileEvent>;

        auto has_base_watch() const -> bool;

        std::filesystem::path const directory;

    private:
        mlc::InotifyWatch base_dir_wd;
        mlc::InotifyWatch override_dir_wd;
        std::set<std::filesystem::path> known_override_files;
    };

    auto apply_events(BatchSummary const&) -> std::optional<miral::live_config::OverridesList>;

private:
    auto find_base_config() const -> std::optional<std::filesystem::path>;
    auto collect_inotify_events() const -> BatchSummary;

    OverrideLoader override_loader;

    std::string const extension;
    std::string const base_config_filename;
    std::string const override_directory_name;

    // Roots ordered by priority: index 0 = lowest priority (last system dir),
    // last index = highest priority (user dir / XDG_CONFIG_HOME)
    std::vector<WatchedRoot> watched_roots;
};
}
}

#endif // MIRAL_OVERRIDE_WATCHER_H
