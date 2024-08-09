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

#include "reloading_config_file.h"

#include <miral/runner.h>

#include <mir/log.h>

#include <boost/throw_exception.hpp>

#include <sys/inotify.h>
#include <unistd.h>

#include <string>
#include <vector>

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

auto watch_fd(mir::Fd const& inotify_fd, std::optional<path> const& path) -> std::optional<mir::Fd>
{
    if (!path.has_value())
        return std::nullopt;

    if (inotify_fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to initialize inotify_fd"}));

    return mir::Fd{inotify_add_watch(inotify_fd, path.value().c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO)};
}
}

miral::ReloadingConfigFile::ReloadingConfigFile(MirRunner& runner, path file, Loader load_config) :
    inotify_fd{inotify_init()},
    load_config{load_config},
    filename{file.filename()},
    directory{config_directory(file)},
    directory_watch_fd{watch_fd(inotify_fd, directory)}
{
    register_handler(runner);

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

    /* Read config file */
    for (auto const& config_root : config_roots)
    {
        auto filepath = config_root / filename;
        if (std::ifstream config_file{filepath})
        {
            load_config(config_file, filepath);
            break;
        }
    }
}

miral::ReloadingConfigFile::~ReloadingConfigFile() = default;

void miral::ReloadingConfigFile::register_handler(MirRunner& runner)
{
    if (directory_watch_fd)
    {
        fd_handle = runner.register_fd_handler(inotify_fd, [icf=inotify_fd, this] (int)
        {
            union
            {
                inotify_event event;
                char buffer[sizeof(inotify_event) + NAME_MAX + 1];
            } inotify_buffer;

            if (read(icf, &inotify_buffer, sizeof(inotify_buffer)) < static_cast<ssize_t>(sizeof(inotify_event)))
                return;

            if (inotify_buffer.event.mask & (IN_CLOSE_WRITE | IN_MOVED_TO))
            try
            {
                if (inotify_buffer.event.name == filename)
                {
                    auto const& file = directory.value() / filename;

                    if (std::ifstream config_file{file})
                    {
                        load_config(config_file, file);
                    }
                }
            }
            catch (...)
            {
                mir::log(mir::logging::Severity::warning, "ReloadingConfigFile", std::current_exception(), "Failed to reload configuration");
            }
        });
    }
}
