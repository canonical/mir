/*
 * Copyright © 2018 Canonical Ltd.
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

#ifndef MIRAL_WAYLAND_EXTENSIONS_H
#define MIRAL_WAYLAND_EXTENSIONS_H

#include <functional>
#include <memory>
#include <string>

struct wl_display;

namespace mir { class Server; }

namespace miral
{
/// Add a user configuration option to Mir's option handling to select
/// the supported Wayland extensions.
/// This adds the command line option '--wayland-extensions' the corresponding
/// MIR_SERVER_WAYLAND_EXTENSIONS environment variable, and the wayland-extensions
/// config line.
/// \remark Since MirAL 2.4
class WaylandExtensions
{
public:
    /// Provide the default extensions supported by Mir
    WaylandExtensions();

    /// Provide a custom set of default extensions (colon separated list)
    /// \note This can only be a subset of supported_extensions().
    explicit WaylandExtensions(std::string const& default_value);

    void operator()(mir::Server& server) const;

    /// All extensions extensions supported by Mir (colon separated list)
    auto supported_extensions() const -> std::string;

    ~WaylandExtensions();
    WaylandExtensions(WaylandExtensions const&);
    auto operator=(WaylandExtensions const&) -> WaylandExtensions&;

    /// \remark Since MirAL 2.5
    using Executor = std::function<void(std::function<void()>&& work)>;

    /// A Builder creates and registers an extension protocol.
    /// The Builder is provided the wl_display so that the extension can be registered and
    /// an executor that allows server initiated code to be executed on the Wayland mainloop.
    /// It returns a shared pointer to the implementation. Mir will manage the lifetime.
    /// \remark Since MirAL 2.5
    using Builder  = std::function<std::shared_ptr<void>(wl_display* display, Executor const& run_on_wayland_mainloop)>;

private:
    struct Self;
    std::shared_ptr<Self> self;

    friend auto with_extension(
        WaylandExtensions const& wayland_extensions,
        std::string const& name, Builder const& builder) -> WaylandExtensions;
};

/// Add a bespoke Wayland extension.
/// \remark Since MirAL 2.5
auto with_extension(
    WaylandExtensions const& wayland_extensions,
    std::string const& name, WaylandExtensions::Builder const& builder) -> WaylandExtensions;

/// Add multiple bespoke Wayland extensions.
/// \remark Since MirAL 2.5
template<typename... Extensions>
auto with_extension(WaylandExtensions const& wayland_extensions,
    std::string const& name, WaylandExtensions::Builder const& builder,
    Extensions... extensions) -> WaylandExtensions
{
    return with_extension(with_extension(wayland_extensions, name, builder), extensions...);
}
}

#endif //MIRAL_WAYLAND_EXTENSIONS_H
