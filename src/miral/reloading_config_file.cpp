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

#include <miral/reloading_config_file.h>

#include <miral/runner.h>

#include <mir/log.h>

#include <boost/throw_exception.hpp>

#include <sys/inotify.h>
#include <unistd.h>

#include <fstream>
#include <optional>
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

auto watch_descriptor(mir::Fd const& inotify_fd, std::optional<path> const& path) -> std::optional<int>
{
    if (!path.has_value())
        return std::nullopt;

    if (inotify_fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to initialize inotify_fd"}));

    return inotify_add_watch(inotify_fd, path.value().c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO);
}
}

class miral::ReloadingConfigFile::Self
{
public:
    Self(MirRunner& runner, path file, Loader load_config);

private:
    mir::Fd const inotify_fd;
    Loader const load_config;
    std::filesystem::path const filename;
    std::optional<std::filesystem::path> const directory;
    std::optional<int> const directory_watch_descriptor;

    void register_handler(MirRunner& runner);
    std::unique_ptr<miral::FdHandle> fd_handle;
};

miral::ReloadingConfigFile::Self::Self(MirRunner& runner, path file, Loader load_config) :
    inotify_fd{inotify_init1(IN_CLOEXEC)},
    load_config{load_config},
    filename{file.filename()},
    directory{config_directory(file)},
    directory_watch_descriptor{watch_descriptor(inotify_fd, directory)}
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

miral::ReloadingConfigFile::ReloadingConfigFile(MirRunner& runner, path file, Loader load_config) :
    self{std::make_shared<Self>(runner, file, load_config)}
{
}

miral::ReloadingConfigFile::~ReloadingConfigFile() = default;

void miral::ReloadingConfigFile::Self::register_handler(MirRunner& runner)
{
    if (directory_watch_descriptor)
    {
        fd_handle = runner.register_fd_handler(inotify_fd, [this] (int)
        {
            static size_t const sizeof_inotify_event = sizeof(inotify_event);

            // A union ensures buffer is aligned for inotify_event
            union
            {
                char buffer[sizeof_inotify_event + NAME_MAX + 1];
                inotify_event unused [[maybe_unused]];
            } inotify_magic;

            auto const readsize = read(inotify_fd, &inotify_magic, sizeof(inotify_magic));
            if (readsize < static_cast<ssize_t>(sizeof_inotify_event))
            {
                return;
            }

            auto raw_buffer = inotify_magic.buffer;
            while (raw_buffer != inotify_magic.buffer + readsize)
            {
                // This is safe because inotify_magic.buffer is aligned and event.len includes padding for alignment
                auto& event = reinterpret_cast<inotify_event&>(*raw_buffer);
                if (event.mask & (IN_CLOSE_WRITE | IN_MOVED_TO) && event.wd == directory_watch_descriptor.value())
                try
                {
                    if (event.name == filename)
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
                    mir::log(mir::logging::Severity::warning, "ReloadingConfigFile", std::current_exception(),
                        "Failed to reload configuration");
                }

                raw_buffer += sizeof_inotify_event+event.len;
            }
        });
    }
}
