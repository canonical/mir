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

#include <filesystem>
#include <miral/config_file.h>

#include <miral/runner.h>

#define MIR_LOG_COMPONENT "ReloadingConfigFile"
#include <mir/log.h>

#include <boost/throw_exception.hpp>

#include <limits.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <fstream>
#include <optional>
#include <string>
#include <sstream>
#include <vector>
#include <ranges>
#include <algorithm>
#include <filesystem>

using namespace std::filesystem;

namespace
{
auto config_directory(path const& file) -> std::optional<path>
{
    if (file.has_parent_path())
    {
        return file.parent_path();
    }
    else if (auto config_home = getenv("XDG_CONFIG_HOME"))
    {
        return config_home;
    }
    else if (auto home = getenv("HOME"))
    {
        return  path(home) / ".config";
    }
    else
    {
        return std::nullopt;
    }
}

auto watch_descriptor(mir::Fd const& inotify_fd, std::optional<path> const& path) -> std::optional<int>
{
    if (!path.has_value())
        return std::nullopt;

    if (inotify_fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to initialize inotify_fd"}));

    auto const ret = inotify_add_watch(
        inotify_fd, path.value().c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM);

    if (ret < 0)
    {
        mir::log_warning("Failed to add inotify watch for '%s': %s", path->c_str(), strerror(errno));
        return std::nullopt;
    }

    return ret;
}

class Watcher : public std::enable_shared_from_this<Watcher>
{
public:
    using Loader = miral::ConfigFile::Loader;
    Watcher(path file, miral::ConfigFile::Loader load_config);

    mir::Fd const inotify_fd;
    Loader const load_config;
    path const filename;
    std::optional<path> const directory;
    std::optional<int> const directory_watch_descriptor;

    void register_handler(miral::MirRunner& runner);
    void handler(int) const;

    std::unique_ptr<miral::FdHandle> fd_handle;
};

class OverrideWatcher : public std::enable_shared_from_this<OverrideWatcher>
{
public:
    using OverrideLoader = miral::ConfigFile::OverrideLoader;
    OverrideWatcher(path base_config, OverrideLoader load_config);

    void register_handler(miral::MirRunner& runner);
private:
    void handler(int);

    mir::Fd const inotify_fd;
    std::unique_ptr<miral::FdHandle> fd_handle;

    OverrideLoader const override_loader;

    std::string const base_config_filename;
    std::string const override_directory;
    std::optional<path> const base_config_directory;

    std::optional<int> base_config_directory_watch_descriptor;
    std::optional<int> override_directory_watch_descriptor;
};

auto get_config_roots(path const& file) -> std::vector<path>
{
    auto const directory{config_directory(file)};

    // With C++26 we should be able to use the optional directory as a range to
    // initialize config_roots.  Until then, we'll just do it the long way...
    std::vector<path> config_roots;

    if (directory)
    {
        config_roots.push_back(directory.value());
    }

    if (auto config_dirs = getenv("XDG_CONFIG_DIRS"))
    {
        std::istringstream config_stream{config_dirs};
        for (std::string config_root; getline(config_stream, config_root, ':');)
        {
            config_roots.push_back(config_root);
        }
    }
    else
    {
        config_roots.push_back("/etc/xdg");
    }

    return config_roots;
}

auto find_config_file(path const& file) -> std::optional<path>
{
    auto const filename{file.filename()};
    auto const config_roots = get_config_roots(file);
    auto const first_existing_filepath = std::ranges::find_if(
        config_roots, [&filename](path const& config_root) { return std::filesystem::exists(config_root / filename); });

    if (first_existing_filepath != config_roots.end())
        return *first_existing_filepath / filename;
    else
        return std::nullopt;
}

auto collect_override_files(path const& override_directory) -> std::vector<path>
{
    auto unsorted =
        directory_iterator{override_directory}
        | std::views::filter([](auto const& f)
                             { return std::filesystem::is_regular_file(f) && f.path().extension() == ".conf"; })
        | std::views::transform([](auto const& f) { return f.path(); }) | std::ranges::to<std::vector>();

    std::sort(unsorted.begin(), unsorted.end());
    return unsorted;
}

auto collect_all_file_streams(path const& config_file) -> std::vector<std::pair<std::unique_ptr<std::istream>, path>>
{
    auto const override_directory = config_file.parent_path() / (config_file.filename().string() + ".d");

    std::vector<std::pair<std::unique_ptr<std::istream>, path>> config_streams;
    config_streams.push_back(std::pair{std::make_unique<std::ifstream>(config_file), config_file});

    if (exists(override_directory))
    {
        auto const override_files = collect_override_files(override_directory);

        for (auto const& override_file : override_files)
        {
            if (std::filesystem::is_regular_file(override_file) && override_file.extension() == ".conf")
                config_streams.emplace_back(std::make_unique<std::ifstream>(override_file), override_file);
        }
    }

    return config_streams;
}

auto watch_override_directory(mir::Fd const& inotify_fd, std::optional<path> const& base_config_directory, std::string const& override_directory) -> std::optional<int>
{
    if (!base_config_directory)
        return std::nullopt;

    auto const override_directory_path = base_config_directory.value() / override_directory;
    if (!exists(override_directory_path))
    {
        mir::log_warning("Override directory '%s' is either does not exist, ignoring", override_directory.c_str());
        return std::nullopt;
    }

    if (!is_directory(override_directory_path))
    {
        mir::log_warning(
            "Override directory path '%s' does not point to a directory, ignoring", override_directory.c_str());
        return std::nullopt;
    }

    return watch_descriptor(inotify_fd, override_directory_path);
}
}

class miral::ConfigFile::Self
{
public:
    Self(MirRunner& runner, path file, Mode mode, Loader load_config);
    Self(MirRunner& runner, path file, Mode mode, OverrideLoader load_config);

private:
    std::shared_ptr<Watcher> watcher;
    std::shared_ptr<OverrideWatcher> override_watcher;
};

Watcher::Watcher(path file, miral::ConfigFile::Loader load_config) :
    inotify_fd{inotify_init1(IN_CLOEXEC)},
    load_config{load_config},
    filename{file.filename()},
    directory{config_directory(file)},
    directory_watch_descriptor{watch_descriptor(inotify_fd, directory)}
{
    if (directory_watch_descriptor.has_value())
    {
        mir::log_debug("Monitoring %s for configuration changes", (directory.value()/filename).c_str());
    }
}

miral::ConfigFile::Self::Self(MirRunner& runner, path file, Mode mode, Loader load_config)
{
    if(auto const config_file = find_config_file(file))
    {
        std::ifstream config_stream{config_file.value()};
        load_config(config_stream, *config_file);
        mir::log_debug("Loaded %s", config_file->c_str());
    }

    switch (mode)
    {
    case Mode::no_reloading:
        break;

    case Mode::reload_on_change:
        watcher = std::make_shared<Watcher>(file, std::move(load_config));
        watcher->register_handler(runner);
        break;
    }
}

miral::ConfigFile::Self::Self(MirRunner& runner, path file, Mode mode, OverrideLoader load_multi_configs)
{
    if(auto const config_file = find_config_file(file))
    {
        auto config_streams = collect_all_file_streams(*config_file);
        load_multi_configs(config_streams);

        auto const files_loaded = config_streams
                                | std::views::transform([](auto const& pair) { return pair.second.string(); })
                                | std::ranges::views::join_with(std::string{", "}) | std::ranges::to<std::string>();
        mir::log_debug("Loaded [%s]", files_loaded.c_str());
    }

    switch (mode)
    {
    case Mode::no_reloading:
        break;

    case Mode::reload_on_change:
        override_watcher = std::make_shared<OverrideWatcher>(file, std::move(load_multi_configs));
        override_watcher->register_handler(runner);
        break;
    }
}

miral::ConfigFile::ConfigFile(MirRunner& runner, path file, Mode mode, Loader load_config) :
    self{std::make_shared<Self>(runner, file, mode, load_config)}
{
}

miral::ConfigFile::ConfigFile(MirRunner& runner, path file, Mode mode, OverrideLoader load_config) :
    self{std::make_shared<Self>(runner, file, mode, load_config)}
{
}

miral::ConfigFile::~ConfigFile() = default;

void Watcher::register_handler(miral::MirRunner& runner)
{
    if (directory_watch_descriptor)
    {
        auto const weak_this = weak_from_this();
        fd_handle = runner.register_fd_handler(inotify_fd, [weak_this] (int fd)
        {
            if (auto const strong_this = weak_this.lock())
            {
                strong_this->handler(fd);
            }
            else
            {
                mir::log_debug("Watcher has been removed, but handler invoked");
            }
        });
    }
}

void Watcher::handler(int) const
{
    static size_t const sizeof_inotify_event = sizeof(inotify_event);

    alignas(inotify_event) char buffer[sizeof(inotify_event) + NAME_MAX + 1];

    auto const readsize = read(inotify_fd, buffer, sizeof(buffer));
    if (readsize < static_cast<ssize_t>(sizeof_inotify_event))
    {
        return;
    }

    auto raw_buffer = buffer;
    while (raw_buffer != buffer + readsize)
    {
        // This is safe because inotify_magic.buffer is aligned and event.len includes padding for alignment
        auto const& event = reinterpret_cast<inotify_event&>(*raw_buffer);
        if (event.mask & (IN_CLOSE_WRITE | IN_MOVED_TO) && event.wd == directory_watch_descriptor.value())
            try
            {
                if (event.name == filename)
                {
                    auto const& file = directory.value() / filename;

                    if (std::ifstream config_file{file})
                    {
                        load_config(config_file, file);
                        mir::log_debug("(Re)loaded %s", file.c_str());
                    }
                    else
                    {
                        mir::log_debug("Failed to open %s", file.c_str());
                    }
                }
            }
        catch (...)
        {
            mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT, std::current_exception(),
                     "Failed to reload configuration");
        }

        raw_buffer += sizeof_inotify_event+event.len;
    }
}

// Let's start simple
//  Watch base config + override directory assuming nothing is a symlink
OverrideWatcher::OverrideWatcher(path base_config, OverrideLoader load_config)
    : inotify_fd{inotify_init1(IN_CLOEXEC)},
      override_loader{load_config},
      base_config_filename{base_config.filename().string()},
      override_directory{base_config_filename + ".d"},
      base_config_directory{config_directory(base_config.filename())},
      base_config_directory_watch_descriptor{watch_descriptor(inotify_fd, base_config_directory)},
      override_directory_watch_descriptor{watch_override_directory(inotify_fd, base_config_directory, override_directory)}
{
}

void OverrideWatcher::register_handler(miral::MirRunner& runner)
{
    if (base_config_directory_watch_descriptor)
    {
        auto const weak_this = weak_from_this();
        fd_handle = runner.register_fd_handler(inotify_fd, [weak_this] (int fd)
        {
            if (auto const strong_this = weak_this.lock())
            {
                strong_this->handler(fd);
            }
            else
            {
                mir::log_debug("OverrideWatcher has been removed, but handler invoked");
            }
        });
    }
}

void OverrideWatcher::handler(int)
{
    static size_t const sizeof_inotify_event = sizeof(inotify_event);

    alignas(inotify_event) char buffer[sizeof(inotify_event) + NAME_MAX + 1];

    auto const readsize = read(inotify_fd, buffer, sizeof(buffer));
    if (readsize < static_cast<ssize_t>(sizeof_inotify_event))
    {
        return;
    }

    auto raw_buffer = buffer;
    while (raw_buffer != buffer + readsize)
    {
        // This is safe because inotify_magic.buffer is aligned and event.len includes padding for alignment
        auto const& event = reinterpret_cast<inotify_event&>(*raw_buffer);

        auto const write_or_move = event.mask & (IN_CLOSE_WRITE | IN_MOVED_TO);
        auto const create = event.mask & IN_CREATE;
        auto const remove = event.mask & (IN_DELETE | IN_MOVED_FROM);

        auto const base_config_changed = write_or_move && base_config_directory_watch_descriptor.has_value()
                                      && event.wd == base_config_directory_watch_descriptor.value()
                                      && event.name == base_config_filename;
        auto const override_file_changed = (write_or_move || create) && override_directory_watch_descriptor.has_value()
                                        && event.wd == override_directory_watch_descriptor.value()
                                        && std::string_view{event.name}.ends_with(".conf");
        auto const override_directory_created = create && base_config_directory_watch_descriptor.has_value()
                                               && event.wd == base_config_directory_watch_descriptor.value()
                                               && event.name == override_directory;
        auto const override_directory_deleted = remove && base_config_directory_watch_descriptor.has_value()
                                               && event.wd == base_config_directory_watch_descriptor.value()
                                               && event.name == override_directory;
        auto const override_file_deleted = remove && override_directory_watch_descriptor.has_value()
                                          && event.wd == override_directory_watch_descriptor.value()
                                          && std::string_view{event.name}.ends_with(".conf");

        if (base_config_changed || override_file_changed || override_directory_created || override_directory_deleted
            || override_file_deleted)
            try
            {
                auto const base_config = base_config_directory.value() / base_config_filename;
                auto const base_config_exists = exists(base_config);

                if (base_config_exists)
                {
                    // Clear and re-establish override directory watch
                    if(override_directory_watch_descriptor)
                    {
                        if (inotify_rm_watch(inotify_fd, *override_directory_watch_descriptor) != 0)
                            mir::log_warning(
                                "Failed to remove watch for override directory '%s': %s",
                                override_directory.c_str(),
                                strerror(errno));

                        override_directory_watch_descriptor =
                            watch_override_directory(inotify_fd, base_config_directory, override_directory);
                    }

                    auto config_streams = collect_all_file_streams(base_config);
                    override_loader(config_streams);

                    auto const files_loaded =
                        config_streams
                        | std::views::transform([](auto const& pair) { return pair.second.string(); })
                        | std::ranges::views::join_with(std::string{", "}) | std::ranges::to<std::string>();
                    mir::log_debug("(Re)loaded [%s]", files_loaded.c_str());
                }
                else
                {
                    mir::log_debug("Failed to open %s", base_config.c_str());
                }
            }
        catch (...)
        {
            mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT, std::current_exception(),
                     "Failed to reload configuration");
        }

        raw_buffer += sizeof_inotify_event+event.len;
    }

}

