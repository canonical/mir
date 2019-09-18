/*
 * Copyright © 2016-2019 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_RUNNER_H
#define MIRAL_RUNNER_H

#include "mir/optional_value.h"

#include <functional>
#include <initializer_list>
#include <memory>

namespace mir { class Server; }

/** Mir Abstraction Layer.
 * A thin, hopefully ABI stable, layer over the Mir libraries exposing only
 * those abstractions needed to write a shell. One day this may inspire a core
 * Mir library.
 */
namespace miral
{

/// Runner for applying initialization options to Mir.
class MirRunner
{
public:
    MirRunner(int argc, char const* argv[]);
    MirRunner(int argc, char const* argv[], char const* config_file);
    ~MirRunner();

    /// Add a callback to be invoked when the server has started,
    /// If multiple callbacks are added they will be invoked in the sequence added.
    void add_start_callback(std::function<void()> const& start_callback);

    /// Add a callback to be invoked when the server is about to stop,
    /// If multiple callbacks are added they will be invoked in the reverse sequence added.
    void add_stop_callback(std::function<void()> const& stop_callback);

    /// Set a handler for exceptions caught in run_with().
    /// run_with() invokes handler() in catch (...) blocks before returning EXIT_FAILURE.
    /// Hence the exception can be re-thrown to retrieve type information.
    /// The default action is to call mir::report_exception(std::cerr)
    void set_exception_handler(std::function<void()> const& handler);

    /// Apply the supplied initialization options and run the Mir server.
    /// @returns EXIT_SUCCESS or EXIT_FAILURE according to whether the server ran successfully
    /// \note blocks until the Mir server exits
    auto run_with(std::initializer_list<std::function<void(::mir::Server&)>> options) -> int;

    /// Tell the Mir server to exit
    void stop();

    /// Name of the .config file.
    /// The .config file is located via the XDG Base Directory Specification:
    ///   $XDG_CONFIG_HOME or $HOME/.config followed by $XDG_CONFIG_DIRS
    /// Config file entries are long form (e.g. "x11-output=1200x720")
    /// \remark Since MirAL 2.4
    auto config_file() const -> std::string;

    /// Name of the .display configuration file.
    /// The .display file is located via the XDG Base Directory Specification:
    ///   $XDG_CONFIG_HOME or $HOME/.config followed by $XDG_CONFIG_DIRS
    /// Config file entries are long form (e.g. "x11-output=1200x720")
    /// \remark Since MirAL 2.4
    auto display_config_file() const -> std::string;

    /// Get the Wayland endpoint name (if any) usable as a $WAYLAND_DISPLAY value
    /// \remark Since MirAL 2.8
    auto wayland_display() const -> mir::optional_value<std::string>;

    /// Get the X11 socket name (if any) usable as a $DISPLAY value
    /// \remark Since MirAL 2.8
    auto x11_display() const -> mir::optional_value<std::string>;

private:
    MirRunner(MirRunner const&) = delete;
    MirRunner& operator=(MirRunner const&) = delete;
    struct Self;
    std::unique_ptr<Self> const self;
};
}

#endif //MIRAL_RUNNER_H
