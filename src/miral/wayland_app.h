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

#ifndef MIRAL_WAYLAND_APP_H
#define MIRAL_WAYLAND_APP_H

#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1.h"
#include <memory>
#include <map>
#include <functional>

/// The MirAL toolkit namespace.
///
/// This namespace provides objects that a developer might use to
/// create an internal client in MirAL.
namespace miral::tk
{
class WaylandApp;

template<typename T>
class WaylandObject
{
public:
    WaylandObject()
        : proxy{nullptr, [](auto){}}
    {
    }

    WaylandObject(T* proxy, void(*destroy)(T*))
        : proxy{proxy, destroy}
    {
    }

    operator T*() const
    {
        return proxy.get();
    }

private:
    std::unique_ptr<T, void(*)(T*)> proxy;
};

/// A wayland client.
///
/// When constructing an internal application, developers may want to
/// bind to the Wayland protocol and interact with the server. This class
/// provides a thin binding to the server in addition to automatic access
/// to commonly used protocols. Users may override #new_global to manage
/// access to other protocols.
class WaylandApp
{
public:
    /// Construct a new wayland application.
    ///
    /// #wayland_init must be called to start establish the connection.
    WaylandApp();
    virtual ~WaylandApp() = default;

    /// Initialize the wayland application on the provided \p display.
    ///
    /// \param display the display
    void wayland_init(wl_display* display);

    /// Execute a blocking roundtrip with the wayland server.
    void roundtrip() const;

    /// \returns a pointer to the display
    auto display() const -> wl_display* { return display_.get(); };

    /// \returns a pointer to the compositor
    auto compositor() const -> wl_compositor* { return compositor_; };

    /// \returns a pointer to the shm interface
    auto shm() const -> wl_shm* { return shm_; }

    /// \returns a pointer to the seat
    auto seat() const -> wl_seat* { return seat_; }

    /// \returns a pointer to the shell
    auto shell() const -> wl_shell* { return shell_; }

    /// \returns a pointer to wlr layer shell
    auto layer_shell() const -> zwlr_layer_shell_v1* { return layer_shell_; };

protected:
    virtual void new_global(wl_registry*, uint32_t, char const*, uint32_t) {}

private:
    /// Doesn't disconnect the display, instead roundtrips it to make sure everything is cleaned up
    std::unique_ptr<wl_display, decltype(&wl_display_roundtrip)> display_;
    WaylandObject<wl_registry> registry_;

    WaylandObject<wl_compositor> compositor_;
    WaylandObject<wl_shm> shm_;
    WaylandObject<wl_seat> seat_;
    WaylandObject<wl_shell> shell_;
    WaylandObject<zwlr_layer_shell_v1> layer_shell_;

    static void handle_new_global(
        void* data,
        struct wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t version);
    static void handle_global_remove(void* data, struct wl_registry* registry, uint32_t name);

    static wl_registry_listener const registry_listener;

    /// Functions to call then drop when globals are removed
    std::map<uint32_t, std::function<void()>> global_remove_handlers;
};
}

#endif // MIRAL_WAYLAND_APP_H
