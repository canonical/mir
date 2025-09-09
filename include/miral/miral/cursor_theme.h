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

#include <string>

namespace mir { class Server; }

namespace miral
{
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
    ///
    /// \param theme A colon-separated list of cursor themes. For example,
    ///              "default:DMZ-White" would specify that the user wishes
    ///              to load the default theme and, as a backup, the DMZ-White
    ///              theme.
    /// \throws std::runtime_error if no theme contains a cursor named "default"
    ///                            with a size of 24x24, this exception will be
    ///                            thrown
    explicit CursorTheme(std::string const& theme);
    ~CursorTheme();

    void operator()(mir::Server& server) const;

private:
    std::string const theme;
};
}


#endif //MIRAL_CURSOR_THEME_H
