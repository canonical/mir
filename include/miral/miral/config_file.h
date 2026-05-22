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

#ifndef MIRAL_CONFIG_FILE_H
#define MIRAL_CONFIG_FILE_H

#include <mir/fd.h>

#include <filesystem>
#include <iosfwd>
#include <string_view>
#include <functional>

namespace miral
{
class MirRunner;
class FdHandle;
namespace live_config { struct OverridesList; }
}

namespace miral
{
/**
 * Utility to locate and monitor a configuration file via the XDG Base Directory
 * Specification. Vis: ($XDG_CONFIG_HOME or $HOME/.config followed by
 * $XDG_CONFIG_DIRS). If, instead of a filename, a path is given, then the base
 * directories are not applied.
 *
 * If mode is `no_reloading`, then the file is loaded on startup and not reloaded
 *
 * If mode is `reload_on_change`, then the file is loaded on startup and either
 * the user-specific configuration file base ($XDG_CONFIG_HOME or $HOME/.config),
 * or the supplied path is monitored for changes.
 * \remark MirAL 5.1
 */
class ConfigFile
{
public:
    /// Loader functor is passed both the open stream and the actual path (for use in reporting problems)
    using Loader = std::function<void(std::istream& istream, std::filesystem::path const& path)>;
    using OverrideLoader = std::move_only_function<void(live_config::OverridesList const&)>;

    /// Mode of reloading
    enum class Mode
    {
        no_reloading,
        reload_on_change
    };

    ConfigFile(MirRunner& runner, std::filesystem::path file, Mode mode, Loader load_config);

    /// Loads the base configuration file together with any override (drop-in)
    /// files from `<config-file>.d/` directories across all XDG config roots.
    /// Files are sorted by basename and deduplicated following systemd
    /// conventions: when multiple roots provide a file with the same basename,
    /// only the highest-priority root's copy is used.
    /// If the base file is not found, \p load_config is not invoked.
    /// \remark since MirAL 5.8
    /// \param extension file extension (including the leading dot) used to filter override files.
    ConfigFile(
        MirRunner& runner,
        std::filesystem::path file,
        Mode mode,
        OverrideLoader load_config,
        std::string_view extension);

    ~ConfigFile();

private:

    class Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_CONFIG_FILE_H
