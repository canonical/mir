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

#ifndef MIRAL_RUNNER_H
#define MIRAL_RUNNER_H

#include "mir/optional_value.h"
#include "mir/fd.h"

#include <functional>
#include <initializer_list>
#include <memory>

namespace mir { class Server; }

/// The Mir Abstraction Layer (mirAL) is an ABI stable layer built on top of the core Mir libraries.
/// This namespace provides constructs such as window management, accessibility features, Wayland
/// extensions, and much more.
///
/// \sa MirRunner - the runner for Mir's engine
/// \sa miral::WindowManagementPolicy - a class that handles all interactions with window management
namespace miral
{
/// A handle which keeps a file descriptor registered to the main loop until it is dropped
struct FdHandle { public: virtual ~FdHandle(); };

/// This class is responsible for managing the lifetime of the core engine
/// of a Mir compositor.
///
/// This is a core class to the lifetime of any Mir-based compositor.
/// Compositor authors will often initialize a single instance of this object
/// at the beginning of their program. They will then use #MirRunner::run_with
/// to begin their server with their desired list of arguments.
class MirRunner
{
public:
    /// Construct a new runner with arguments provided from the commandline.
    ///
    /// \param argc number of arguments
    /// \param argv array of arguments
    MirRunner(int argc, char const* argv[]);

    /// Construct a new runner with arguments provided from the commandline as
    /// well as a path to a Mir config file.
    ///
    /// \param argc number of arguments
    /// \param argv array of arguments
    /// \param config_file path to a config file
    MirRunner(int argc, char const* argv[], char const* config_file);
    ~MirRunner();

    /// Add a callback to be invoked when the server has started.
    ///
    /// If multiple callbacks are added, they will be invoked in the sequence added.
    ///
    /// \param start_callback callback to be invoked
    void add_start_callback(std::function<void()> const& start_callback);

    /// Add a callback to be invoked when the server is about to stop.
    ///
    /// If multiple callbacks are added they will be invoked in the reverse sequence added.
    ///
    /// \param stop_callback callback to be invoked
    void add_stop_callback(std::function<void()> const& stop_callback);

    /// Add signal handler to the server's main loop.
    ///
    /// \param signals a list of signals to listen on
    /// \param handler a handler function called when a signal occurs
    void register_signal_handler(
        std::initializer_list<int> signals,
        std::function<void(int)> const& handler);

    /// Add a watch on a file descriptor to the server's main loop.
    ///
    /// The handler will be triggered when there is data to read on \p fd.
    ///
    /// \param fd the file descriptor to wait on
    /// \param handler a handler function called when data is available
    /// \returns a handle
    auto register_fd_handler(
        mir::Fd fd,
        std::function<void(int)> const& handler)
    -> std::unique_ptr<miral::FdHandle>;

    /// Set a handler for exceptions caught in #MirRunner::run_with().
    ///
    /// #run_with() invokes \p handler in `catch (...)` blocks before returning `EXIT_FAILURE`.
    /// Hence, the exception can be re-thrown to retrieve type information.
    ///
    /// The default action is to call `mir::report_exception(std::cerr)`.
    ///
    /// \param handler the handler
    void set_exception_handler(std::function<void()> const& handler);

    /// Apply the supplied initialization options and run the Mir server.
    ///
    /// This method blocks until the server exits.
    ///
    /// The supplied \p options parameter is an initializer list of functions that take a #mir::Server
    /// as an argument. Throughout the `miral` namespace, you will see classes implement
    /// `void operator()(mir::Server&)` so that they may be implicitly passed to this method.
    ///
    /// \param options an initializer list of functions that take a #mir::Server as an argument
    /// \returns `EXIT_SUCCESS` or `EXIT_FAILURE` according to whether the server ran successfully
    auto run_with(std::initializer_list<std::function<void(::mir::Server&)>> options) -> int;

    /// Tell the Mir server to exit.
    void stop();

    /// Name of the .config file.
    ///
    /// The .config file is located via the XDG Base Directory Specification:
    ///   `$XDG_CONFIG_HOME` or `$HOME/.config` followed by `$XDG_CONFIG_DIRS`
    ///
    /// Config file entries are long form (e.g. "x11-output=1200x720").
    ///
    /// \returns name of the config file
    auto config_file() const -> std::string;

    /// Name of the .display configuration file.
    ///
    /// The .display file is located via the XDG Base Directory Specification:
    ///   `$XDG_CONFIG_HOME` or `$HOME/.config` followed by `$XDG_CONFIG_DIRS`
    ///
    /// Config file entries are long form (e.g. "x11-output=1200x720")
    ///
    /// \returns name of the display config file
    auto display_config_file() const -> std::string;

    /// Get the Wayland socket name.
    ///
    /// This value is usable as a `$WAYLAND_DISPLAY` value.
    ///
    /// This will be an empty optional if the server has not yet started.
    ///
    /// \returns the wayland socket, if any
    auto wayland_display() const -> mir::optional_value<std::string>;

    /// Get the X11 socket name.
    ///
    /// This is usable as a `$DISPLAY` value.
    ///
    /// This will be an empty optional if the server has not yet started or X11 is not
    /// enabled for the server.
    ///
    /// \returns the X11 socket, if any
    /// \sa miral::X11Support - provides X11 support to a Mir server
    auto x11_display() const -> mir::optional_value<std::string>;

private:
    MirRunner(MirRunner const&) = delete;
    MirRunner& operator=(MirRunner const&) = delete;
    struct Self;
    std::unique_ptr<Self> const self;
};
}

#endif //MIRAL_RUNNER_H
