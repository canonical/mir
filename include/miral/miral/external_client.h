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
class ExternalClientLauncher
{
public:
    ExternalClientLauncher();
    ~ExternalClientLauncher();

    void operator()(mir::Server& server);

    /// Launch with client environment configured for Wayland.
    /// If X11 is enabled, then DISPLAY will also be set accordingly.
    /// \return The pid of the process that was launched.
    /// \remark Return type changed from void in MirAL 3.0
    auto launch(std::vector<std::string> const& command_line) const -> pid_t;

    /// If X11 is enabled, then launch with client environment configured for X11.
    /// For the occasions it is desired to coerce applications into using X11
    /// \return The pid of the process that was launched (or -1 if X11 is not enabled)
    /// \remark Return type changed from void in MirAL 3.0
    auto launch_using_x11(std::vector<std::string> const& command_line) const -> pid_t;

    /// Use the proposed `desktop-entry` snap interface to launch another snap
    void snapcraft_launch(std::string const& desktop_file) const;

    /// Split out the tokens of a (possibly escaped) command
    static auto split_command(std::string const& command) -> std::vector<std::string>;

    /// Launch with client environment configured for Wayland.
    /// If X11 is enabled, then DISPLAY will also be set accordingly.
    /// \return The pid of the process that was launched.
    auto launch(std::string const& command) const -> pid_t;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_EXTERNAL_CLIENT_H
