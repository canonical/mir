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

#include "miral/config_file.h"
#include "live_config_watcher.h"

#include <mir/log.h>
#include <miral/runner.h>

#include <sys/inotify.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace fs = std::filesystem;
namespace mlc = miral::live_config;
using path = fs::path;

namespace
{
auto find_config_file(std::vector<path> const& config_roots, path const& filename) -> std::optional<path>
{
    auto const it = std::ranges::find_if(
        config_roots,
        [&filename](path const& config_root)
        {
            std::error_code ec; // Don't care about the specific error. Likely "file not found"
            return fs::exists(config_root / filename, ec);
        });

    if (it != config_roots.end())
        return *it / filename;
    else
        return std::nullopt;
}

class SingleFileWatcher : public mlc::Watcher
{
public:
    using Loader = miral::ConfigFile::Loader;
    SingleFileWatcher(path file, miral::ConfigFile::Loader load_config);
    void handler(int) override;

protected:
    bool should_register() const override
    {
        return static_cast<bool>(directory_watch_descriptor);
    }

private:
    Loader const load_config;
    path const filename;
    std::optional<path> const directory;
    mlc::InotifyWatch const directory_watch_descriptor;
};
}

class miral::ConfigFile::Self
{
public:
    Self(MirRunner& runner, path file, Mode mode, Loader load_config);

private:
    std::shared_ptr<mlc::Watcher> watcher;
};

miral::ConfigFile::Self::Self(MirRunner& runner, path file, Mode mode, Loader load_config)
{
    auto const config_roots = mlc::get_config_roots(file);
    if (auto const config_file = find_config_file(config_roots, file.filename()))
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
        watcher = std::make_shared<SingleFileWatcher>(file, std::move(load_config));
        watcher->register_handler(runner);
        break;
    }
}

miral::ConfigFile::ConfigFile(MirRunner& runner, path file, Mode mode, Loader load_config) :
    self{std::make_shared<Self>(runner, file, mode, load_config)}
{
}

miral::ConfigFile::~ConfigFile() = default;

SingleFileWatcher::SingleFileWatcher(path file, miral::ConfigFile::Loader load_config) :
    load_config{load_config},
    filename{file.filename()},
    directory{mlc::config_directory(file)},
    directory_watch_descriptor{directory ? mlc::InotifyWatch{inotify_fd, *directory} : mlc::InotifyWatch{}}
{
    if (directory_watch_descriptor)
    {
        mir::log_debug("Monitoring %s for configuration changes", (directory.value() / filename).c_str());
    }
}

void SingleFileWatcher::handler(int)
{
    for_each_inotify_event(
        [this](inotify_event const& event)
        {
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
                    mir::log(
                        mir::logging::Severity::warning,
                        MIR_LOG_COMPONENT,
                        std::current_exception(),
                        "Failed to reload configuration");
                }
        });
}
