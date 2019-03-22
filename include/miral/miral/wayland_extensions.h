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
    class Context
    {
    public:
        virtual auto display() const -> wl_display* = 0;
        virtual void run_on_wayland_mainloop(std::function<void()>&& work) const = 0;

    protected:
        Context() = default;
        virtual ~Context() = default;
        Context(Context const&) = delete;
        Context& operator=(Context const&) = delete;
    };

    /// A Builder creates and registers an extension protocol.
    /// \remark Since MirAL 2.5
    struct Builder
    {
        /// Name of the protocol extension
        std::string name;

        /// Functor that creates and registers an extension protocol
        /// \param context giving access to:
        ///   * the wl_display (so that, for example, the extension can be registered); and,
        ///   * allowing server initiated code to be executed on the Wayland mainloop.
        /// \return a shared pointer to the implementation. (Mir will manage the lifetime)
        std::function<std::shared_ptr<void>(Context const* context)> build;
    };

    /// \remark Since MirAL 2.5
    using Filter = std::function<bool(Application const& app, char const* protocol)>;

    /// Set an extension filter callback to control the extensions available to specific clients
    /// \remark Since MirAL 2.5
    void set_filter(Filter const& extension_filter);

    /// Add a bespoke Wayland extension.
    /// \remark Since MirAL 2.5
    void with_extension(Builder const& builder);

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_WAYLAND_EXTENSIONS_H
