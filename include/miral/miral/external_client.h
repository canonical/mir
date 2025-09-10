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

#ifndef MIRAL_EXTERNAL_CLIENT_H
#define MIRAL_EXTERNAL_CLIENT_H

#include <sys/types.h>

#include <memory>
#include <string>
#include <vector>

namespace mir { class Server; }

namespace miral
{
/// This class provides methods to launch external clients from the compositor.
///
/// This class launches clients with the correct environment variables (e.g.
/// WAYLAND_DISPLAY) as well as a valid activation token.
class ExternalClientLauncher
{
public:
    /// Construct a new external client launcher.
    ExternalClientLauncher();
    ~ExternalClientLauncher();

    void operator()(mir::Server& server);

    /// Launch the provided command with an environment configured for Wayland.
    ///
    /// If X11 is enabled, then DISPLAY will also be set accordingly.
    ///
    /// \param command_line the command to launch
    /// \returns The pid of the process that was launched.
    /// \pre the server has started with this instance passed to #MirRunner::run_with().
    auto launch(std::vector<std::string> const& command_line) const -> pid_t;

    /// If X11 is enabled, then launch with an environment configured for X11.
    ///
    /// This is useful in occasions where it is desired to coerce applications into using X11
    ///
    /// If X11 is unavailable, this method always returns `-1`.
    ///
    /// \returns The pid of the process that was launched, or -1 if X11 is not enabled
    /// \pre the server has started with this instance passed to #MirRunner::run_with().
    auto launch_using_x11(std::vector<std::string> const& command_line) const -> pid_t;

    /// Use the proposed `desktop-entry` snap interface to launch a snap.
    ///
    /// \param desktop_file the desktop file name of the snap
    /// \pre the server has started with this instance passed to #MirRunner::run_with().
    void snapcraft_launch(std::string const& desktop_file) const;

    /// Split out the tokens of an escaped \p command.
    ///
    /// \param command an unsplit command
    /// \returns a command split into a `std::vector`
    static auto split_command(std::string const& command) -> std::vector<std::string>;

    /// Launch the \p command with an environment configured for Wayland.
    ///
    /// If X11 is enabled, then DISPLAY will also be set accordingly.
    ///
    /// \param command the command to run
    /// \return The pid of the process that was launched.
    /// \pre the server has started with this instance passed to #MirRunner::run_with().
    auto launch(std::string const& command) const -> pid_t;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_EXTERNAL_CLIENT_H
