/*
 * Copyright © 2018-2019 Canonical Ltd.
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

#include "application.h"

#include <functional>
#include <memory>
#include <string>

struct wl_display;

namespace mir { class Server; }

namespace miral
{
class WaylandExtensions;

/// A Wayland protocol extension
/// Not connected to any specific Wayland display
/// an instance of this extension is created for a specific display before it can have effect
/// \remark Since MirAL 2.5
class WaylandExtension
{
public:
    /// The context provides what an extension needs to create an instance
    class Context
    {
    public:
        auto wayland_display() const -> wl_display*;
        void run_on_wayland_mainloop(std::function<void()>&& work) const;

    private:
        struct Self;
        std::unique_ptr<Self> self;
        Context(std::unique_ptr<Self> self);
        friend WaylandExtensions;
    };

    /// An instance of this extension, connected to a specific Wayland display
    class Instance
    {
    };

    /// The name of the implemented Wayland protocol interface
    virtual auto interface_name() const -> std::string = 0;

    /// Create an instance for the given context
    virtual auto instantiate_for(Context const& context) const -> std::shared_ptr<Instance> = 0;

    virtual ~WaylandExtension() = default;
};

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

    /// Add multiple bespoke Wayland extensions.
    /// \remark Since MirAL 2.5
    auto with_extensions(
        std::initializer_list<std::shared_ptr<WaylandExtension>> extensions) const -> WaylandExtensions;

    /// \remark Since MirAL 2.5
    using Filter = std::function<bool(Application const& app, char const* protocol)>;

    /// Set an extension filter callback to control the extensions available to specific clients
    /// \remark Since MirAL 2.5
    void set_filter(Filter const& extension_filter);

private:
    struct Self;
    std::shared_ptr<Self> self;
};

/// Set an extension filter callback to control the extensions available to specific clients.
/// \remark Since MirAL 2.5
inline auto with_filter(
    WaylandExtensions wayland_extensions,
    WaylandExtensions::Filter const& extension_filter) -> WaylandExtensions
{
    wayland_extensions.set_filter(extension_filter);
    return wayland_extensions;
}
}

#endif //MIRAL_WAYLAND_EXTENSIONS_H
