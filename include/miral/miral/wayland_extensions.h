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
    auto all_supported() const -> std::set<std::string>;

    /// Context information useful for implementing Wayland extensions
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
    struct Builder
    {
        /// Name of the protocol extension's Wayland global
        std::string name;

        /// Functor that creates and registers an extension protocol
        /// \param context giving access to:
        ///   * the wl_display (so that, for example, the extension can be registered); and,
        ///   * allowing server initiated code to be executed on the Wayland mainloop.
        /// \return a shared pointer to the implementation. (Mir will manage the lifetime)
        std::function<std::shared_ptr<void>(Context const* context)> build;
    };

    /// Information that can be used to determine if to enable a conditionally enabled extension
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

    using Filter = std::function<bool(Application const& app, char const* protocol)>;

    using EnableCallback = std::function<bool(EnableInfo const& info)>;

    /**
     * Supported wayland extensions that are not enabled by default.
     * These can be passed into WaylandExtensions::enable() to turn them on.
     * @{ */

    /// Enables shell components such as panels, notifications and lock screens.
    /// It is recommended to use this in conjunction with conditionally_enable() as malicious
    /// clients could potentially use this protocol to steal input focus or
    /// otherwise bother the user.
    static char const* const zwlr_layer_shell_v1;

    /// Allows clients to retrieve additional information about outputs
    static char const* const zxdg_output_manager_v1;

    /// Allows a client to get information and gain control over all toplevels of all clients
    /// Useful for taskbars and app switchers
    /// Could allow a client to extract information about other programs the user is running
    static char const* const zwlr_foreign_toplevel_manager_v1;

    /// Allows clients to act as a virtual keyboard, useful for on-screen keyboards.
    /// Clients are not required to display anything to send keyboard events using this extension,
    /// so malicious clients could use it to take actions without user input.
    static char const* const zwp_virtual_keyboard_manager_v1;

    /// Allows clients (such as on-screen keyboards) to intercept physical key events and act as a
    /// source of text input for other clients. Input methods are not required to display anything
    /// to use this extension, so malicious clients could use it to intercept keys events or take
    /// actions without user input.
    static const char* const zwp_input_method_v1;

    /// Allows clients to display a surface as an input panel surface. The input panel surface
    /// is shown when a text input is active and hidden otherwise. The panel itself can either
    /// be attached to the edge of the screen or set to float near the active input. This is
    /// often used in conjunction with zwp_input_method_v1.
    static const char* const zwp_input_panel_v1;

    /// Allows clients (such as on-screen keyboards) to intercept physical key events and act as a
    /// source of text input for other clients. Input methods are not required to display anything
    /// to use this extension, so malicious clients could use it to intercept keys events or take
    /// actions without user input.
    static char const* const zwp_input_method_manager_v2;

    /// Allows clients to take screenshots and record the screen. Only enable for clients that are
    /// trusted to view all displayed content, including windows of other apps.
    static char const* const zwlr_screencopy_manager_v1;

    /// Allows clients to act as a virtual pointer, useful for remote control and automation.
    /// Clients are not required to display anything to send pointer events using this extension,
    /// so malicious clients could use it to take actions without user input.
    static char const* const zwlr_virtual_pointer_manager_v1;

    /// Allows clients to act as a screen lock.
    static char const* const ext_session_lock_manager_v1;

    /// Add a bespoke Wayland extension both to "supported" and "enabled by default".
    void add_extension(Builder const& builder);

    /// Add a bespoke Wayland extension both to "supported" but not "enabled by default".
    void add_extension_disabled_by_default(Builder const& builder);

    /// The set of Wayland extensions that Mir recommends.
    /// Also the set that is enabled by default upon construction of a WaylandExtensions object.
    static auto recommended() -> std::set<std::string>;

    /// The set of Wayland extensions that core Mir supports.
    /// Does not include bespoke extensions
    /// A superset of recommended()
    static auto supported() -> std::set<std::string>;

    /// Enable a Wayland extension by default. The user can still disable it with the drop-wayland-extensions or
    /// wayland-extensions options. The extension can be forced to be enabled regardless of user options with
    /// conditionally_enable().
    auto enable(std::string name) -> WaylandExtensions&;

    /// Disable a Wayland extension by default. The user can still enable it with the add-wayland-extensions or
    /// wayland-extensions options. The extension can be forced to be disabled regardless of user options with
    /// conditionally_enable().
    auto disable(std::string name) -> WaylandExtensions&;

    /// Enable a Wayland extension only when the \p callback returns true.
    ///
    /// The callback can use info.user_preference() to respect the extension options the user provided, although it is not required.
    /// Unlike #enable and #disable, #conditionally_enable can override the user options. The callback may be
    /// called multiple times for each  client/extension pair (for example, once each time a client creates or
    /// destroys a wl_registry).
    ///
    /// All client processing will be blocked while the callback is being executed. To minimize the impact on client
    /// responsiveness, users may want to cache the result of any expensive checks made in the callback.
    ///
    /// This method may be called multiple times for the same extension \p name. The \p callback functions
    /// will be executed in the order in which they are added until one returns `true` or they all return `false`.
    auto conditionally_enable(std::string name, EnableCallback const& callback) -> WaylandExtensions&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

/// Get the MirAL application for a wl_client.
/// \return The application (null if no application is found)
auto application_for(wl_client* client) -> Application;

/// Get the MirAL application for a wl_resource.
/// \return The application (null if no application is found)
auto application_for(wl_resource* resource) -> Application;

/// Get the MirAL Window for a Wayland Surface, XdgSurface, etc.
/// Note that there may not be a corresponding miral::Window (e.g. the
/// surface is created and assigned properties before 'commit' creates the
/// miral::Window).
///
/// \return The window (null if no window is found)
auto window_for(wl_resource* surface) -> Window;
}

#endif //MIRAL_WAYLAND_EXTENSIONS_H
