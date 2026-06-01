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
 * Utility to locate and monitor either a singular configuration file, or a base
 * configuration file and its overrides.
 * \remark MirAL 5.1
 */
class ConfigFile
{
public:
    /// Loader functor is passed both the open stream and the actual path (for use in reporting problems)
    using Loader = std::function<void(std::istream& istream, std::filesystem::path const& path)>;
    /// Provides access to OverridesList which allows access to the full list of
    /// unchanged, modified, added, and dropped files.
    using OverrideLoader = std::move_only_function<void(live_config::OverridesList const&)>;

    /// Mode of reloading
    ///
    /// `no_reloading`: files are loaded on startup and not reloaded
    ///
    /// `reload_on_change`: files are loaded on startup and when either the
    /// user-specific configuration file base ($XDG_CONFIG_HOME or
    /// $HOME/.config), or the supplied path that is monitored for changes.
    enum class Mode
    {
        no_reloading,
        reload_on_change
    };

    /// Utility to locate and monitor a configuration file via the XDG Base Directory
    /// Specification. Vis: ($XDG_CONFIG_HOME or $HOME/.config followed by
    /// $XDG_CONFIG_DIRS). If, instead of a filename, a path is given, then the base
    /// directories are not applied.
    ConfigFile(MirRunner& runner, std::filesystem::path file, Mode mode, Loader load_config);

    /// Loads the base configuration file together with any override (drop-in)
    /// files from `<base-config-name>.d/` directories across all XDG config
    /// roots.
    ///
    /// If the \p base_config is only a filename and not a path, finds the
    /// highest-priority file with that name in `$XDG_CONFIG_HOME`,
    /// `$HOME/.config`, and `$XDG_CONFIG_DIRS` respectively. Otherwise, the
    /// base config is used as is.
    ///
    /// If the base config file is not found, \p load_config is not invoked.
    ///
    /// Override files are sorted lexicographically by basename. If multiple
    /// roots provide an override drop-in file with the same basename, only the
    /// highest-priority root's copy is used. Root priority follows the order
    /// used in the base config search process: `$XDG_CONFIG_HOME` >
    /// `$HOME/.config` > `$XDG_CONFIG_DIRS` (in order).
    ///
    /// Files can be added or removed at runtime in any of the aforementioned
    /// configuration roots. In this case, priority is re-evaluated, and higher
    /// priority base config or override files take precedence over lower
    /// priority ones. If a higher priority file is removed, the next-highest
    /// priority file is used instead.
    ///
    /// \remark since MirAL 5.8
    /// \param base_config the base configuration file, either as a filename or
    ///                    a path.
    /// \param load_config callback used in the initial load and to signal a
    ///                    change in one or more of the config files. The full
    ///                    list of files unchanged, modified, added, and dropped
    ///                    is passed through OverridesList.
    /// \param extension file extension (including the leading dot) used to
    ///                  filter override files.
    /// \see miral::live_config::OverridesList
    ConfigFile(
        MirRunner& runner,
        std::filesystem::path base_config,
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
