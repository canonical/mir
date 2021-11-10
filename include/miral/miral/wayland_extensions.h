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
#include <optional>
#include <set>

struct wl_display;
struct wl_client;
struct wl_resource;

namespace mir { class Server; }

namespace miral
{
class Window;

/// Enable configuration of the Wayland extensions enabled at runtime.
///
/// This adds the command line options '--wayland-extensions', '--add-wayland-extensions', '--drop-wayland-extensions'
/// and the corresponding MIR_SERVER_* environment variables and config file options.
///   * The server can add support for additional extensions
///   * The server can specify the configuration defaults
///   * Mir's option handling allows the defaults to be overridden
/// \remark Since MirAL 2.4
class WaylandExtensions
{
public:
    /// Default to enabling the extensions recommended by Mir
    WaylandExtensions();

    void operator()(mir::Server& server) const;

    ~WaylandExtensions();
    WaylandExtensions(WaylandExtensions const&);
    auto operator=(WaylandExtensions const&) -> WaylandExtensions&;

    /// All Wayland extensions supported.
    /// This includes both the supported() provided by Mir and any extensions
    /// that have been added using add_extension().
    /// \remark Since MirAL 3.0
    auto all_supported() const -> std::set<std::string>;

    /// Context information useful for implementing Wayland extensions
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

    /// Information that can be used to determine if to enable a conditionally enabled extension
    /// \remark Since MirAL 3.4
    class EnableInfo
    {
    public:
        /// The application that is being given access to this extension
        auto app() const -> Application const&;
        /// The name of the extension/global, always the same as given to conditionally_enable()
        auto name() const -> const char*;
        /// If the user has enabled or disabled this extension one of the wayland extension Mir options
        auto user_preference() const -> std::optional<bool>;

    private:
        friend WaylandExtensions;
        EnableInfo(Application const& app, const char* name, std::optional<bool> user_preference);
        struct Self;
        std::unique_ptr<Self> const self;
    };

    /// \remark Since MirAL 2.5
    using Filter = std::function<bool(Application const& app, char const* protocol)>;

    /// \remark Since MirAL 3.4
    using EnableCallback = std::function<bool(EnableInfo const& info)>;

    /// Set an extension filter callback to control the extensions available to specific clients. Deprecated in favor of
    /// conditionally_enable(), and not be used in conjunction with conditionally_enable().
    /// \remark Since MirAL 2.5
    /// \deprecated In MirAL 3.4, use conditionally_enable() instead
    void set_filter(Filter const& extension_filter);

    /**
     * Supported wayland extensions that are not enabled by default.
     * These can be passed into WaylandExtensions::enable() to turn them on.
     * @{ */

    /// Enables shell components such as panels, notifications and lock screens.
    /// It is recommended to use this in conjunction with set_filter() as malicious
    /// clients could potentially use this protocol to steal input focus or
    /// otherwise bother the user.
    /// \remark Since MirAL 2.6
    static char const* const zwlr_layer_shell_v1;

    /// Allows clients to retrieve additional information about outputs
    /// \remark Since MirAL 2.6
    static char const* const zxdg_output_manager_v1;

    /// Allows a client to get information and gain control over all toplevels of all clients
    /// Useful for taskbars and app switchers
    /// Could allow a client to extract information about other programs the user is running
    /// \remark Since MirAL 3.1
    static char const* const zwlr_foreign_toplevel_manager_v1;

    /// Allows clients to act as a virtual keyboard, useful for on-screen keyboards.
    /// Clients are not required to display anything to send keyboard events using this extension,
    /// so malicious clients could use it to take actions without user input.
    /// \remark Since MirAL 3.4
    static char const* const zwp_virtual_keyboard_manager_v1;


    /// Allows clients (such as on-screen keyboards) to intercept physical key events and act as a
    /// source of text input for other clients. Input methods are not required to display anything
    /// to use this extension, so malicious clients could use it to intercept keys events or take
    /// actions without user input.
    /// \remark Since MirAL 3.4
    static char const* const zwp_input_method_manager_v2;

    /**
     * \remark Since MirAL 3.3
     * \deprecated Use the *_manager_* versions instead
     * @{ */
    static char const* const zwp_virtual_keyboard_v1;
    static char const* const zwp_input_method_v2;
    /** @} */
    /** @} */

    /// Add a bespoke Wayland extension both to "supported" and "enabled by default".
    /// \remark Since MirAL 2.5
    void add_extension(Builder const& builder);

    /// Add a bespoke Wayland extension both to "supported" but not "enabled by default".
    /// \remark Since MirAL 2.5
    void add_extension_disabled_by_default(Builder const& builder);

    /// The set of Wayland extensions that Mir recommends.
    /// Also the set that is enabled by default upon construction of a WaylandExtensions object.
    /// \remark Since MirAL 2.6
    static auto recommended() -> std::set<std::string>;

    /// The set of Wayland extensions that core Mir supports.
    /// Does not include bespoke extensions
    /// A superset of recommended()
    /// \remark Since MirAL 2.6
    static auto supported() -> std::set<std::string>;

    /// Enable a Wayland extension by default. The user can still disable it with the drop-wayland-extensions or
    /// wayland-extensions options. The extension can be forced to be enabled regardless of user options with
    /// conditionally_enable().
    /// \remark Since MirAL 2.6
    auto enable(std::string name) -> WaylandExtensions&;

    /// Disable a Wayland extension by default. The user can still enable it with the add-wayland-extensions or
    /// wayland-extensions options. The extension can be forced to be disabled regardless of user options with
    /// conditionally_enable().
    /// \remark Since MirAL 2.6
    auto disable(std::string name) -> WaylandExtensions&;

    /// Enable a Wayland extension only when the callback returns true. The callback can use info.user_preference()
    /// to respect the extension options the user provided, it is not required. Unlike enable() and disable(),
    /// conditionally_enable() can override the user options.
    /// \remark Since MirAL 3.4
    auto conditionally_enable(std::string name, EnableCallback const& callback) -> WaylandExtensions&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

/// Get the MirAL application for a wl_client.
/// \return The application (null if no application is found)
/// \remark Since MirAL 2.5
auto application_for(wl_client* client) -> Application;

/// Get the MirAL application for a wl_resource.
/// \return The application (null if no application is found)
/// \remark Since MirAL 2.5
auto application_for(wl_resource* resource) -> Application;

/// Get the MirAL Window for a Wayland Surface, XdgSurface, etc.
/// Note that there may not be a corresponding miral::Window (e.g. the
/// surface is created and assigned properties before 'commit' creates the
/// miral::Window).
///
/// \return The window (null if no window is found)
/// \remark Since MirAL 2.5
auto window_for(wl_resource* surface) -> Window;
}

#endif //MIRAL_WAYLAND_EXTENSIONS_H
