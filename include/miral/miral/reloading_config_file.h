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

#ifndef MIRAL_RELOADING_CONFIG_FILE_H
#define MIRAL_RELOADING_CONFIG_FILE_H

#include <mir/fd.h>

#include <filesystem>
#include <istream>
#include <functional>

namespace miral { class MirRunner; class FdHandle; }

namespace miral
{
/**
 * Utility to locate and monitor a config file via
 * the XDG Base Directory Specification. Vis:
 * ($XDG_CONFIG_HOME or $HOME/.config followed by $XDG_CONFIG_DIRS)
 * If, instead of a filename, a path is given, then that is used instead
 * \remark MirAL 5.1
 */
class ReloadingConfigFile
{
public:
    /// Loader functor is passed both the open stream and the actual path (for use in reporting problems)
    using Loader = std::function<void(std::istream& istream, std::filesystem::path const& path)>;

    ReloadingConfigFile(MirRunner& runner, std::filesystem::path file, Loader load_config);
    ~ReloadingConfigFile();

private:

    class Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_RELOADING_CONFIG_FILE_H