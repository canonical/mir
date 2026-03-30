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

#ifndef MIRAL_CURSOR_THEME_H
#define MIRAL_CURSOR_THEME_H

#include <memory>
#include <string>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }

/// Loads a cursor theme.
///
/// Cursor themes are specified as a colon-separated list of theme names.
/// Mir will search each theme in order and use the first one it can load.
///
/// A theme specified via the `--cursor-theme` command-line option takes
/// precedence over the constructor parameter.
///
/// Example:
/// \code
/// "default:DMZ-White"
/// \endcode
///
/// This tries to load the `default` theme first and falls back to `DMZ-White`
/// if `default` is unavailable.
class CursorTheme
{
public:
    /// Construct a cursor theme with the provided set of themes for the cursor.
    ///
    /// \param theme A colon-separated list of cursor themes. For example,
    ///              "default:DMZ-White" would specify that the user wishes
    ///              to load the default theme and, as a backup, the DMZ-White
    ///              theme.
    /// \throws std::runtime_error if no theme contains a cursor named "default"
    ///                            with a size of 24x24, this exception will be
    ///                            thrown
    explicit CursorTheme(std::string const& theme);

    /// Construct registering with a configuration store.
    ///
    /// Available options:
    ///     - {cursor, theme}: Sets the cursor theme at runtime. The value is
    ///       a colon-separated list of theme names.
    ///
    /// \param theme default colon-separated cursor theme list
    /// \param config_store the config store
    /// \remark Since MirAL 5.8
    CursorTheme(std::string const& theme, live_config::Store& config_store);

    ~CursorTheme();

    void operator()(mir::Server& server) const;

    /// Change the cursor theme at runtime.
    ///
    /// \param theme A colon-separated list of cursor themes.
    /// \remark Since MirAL 5.8
    void set_theme(std::string const& theme) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}


#endif //MIRAL_CURSOR_THEME_H
