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

#ifndef MIRAL_INTERNAL_CLIENT_H
#define MIRAL_INTERNAL_CLIENT_H

#include <functional>
#include <memory>
#include <string>

namespace mir { class Server; namespace scene { class Session; }}

struct wl_display;

namespace miral
{
/** Wrapper for running an internal Mir client at startup
 *  \note client_code will be executed on its own thread, this must exit
 *  \note connection_notification will be called on a worker thread and must not block
 *
 *  \param client_code              code implementing the internal client
 *  \param connection_notification  handler for registering the server-side application
 */

/// Wraps an internal client such that it runs at startup and does not need to
/// be launched with the #miral::InternalClientLauncher.
class StartupInternalClient
{
public:
    /// Construct an internal client to be launched when the Mir server starts.
    ///
    /// The \p connect_notification call will be called when a connection to the server has
    /// been secured. This is called on a worker thread and must not block.
    ///
    /// The \p client_code callback will be called when the client has been initialized and is ready
    /// to start interacting with the server. Note that this callback happens on another thread.
    /// This callback must exit.
    ///
    /// \param client_code called when the Wayland client is initialized
    /// \param connect_notification called when the session has connected
    explicit StartupInternalClient(
        std::function<void(struct ::wl_display* display)> client_code,
        std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification);

    /// Construct an internal client to be launched when the Mir server starts.
    ///
    /// The \p client_object instance must define:
    /// - `operator()(struct wl_display*)` - the method called when the client has been initialized
    ///                                      and is ready to start interacting with the server. Note that
    ///                                      this callback happens in another thread. This callback must exit.
    /// - `operator()(std::weak_ptr<mir::scene::Session> const)` - the method called when a connection to the server
    ///                                                            has been secured. This is called on a worker thread
    ///                                                            and must not block.
    ///
    /// \param client_object an object describing the client connection
    template <typename ClientObject>
    explicit StartupInternalClient(ClientObject const& client_object) :
        StartupInternalClient(client_object, client_object) {}

    ~StartupInternalClient();

    void operator()(mir::Server& server);

private:
    class Self;
    std::shared_ptr<Self> internal_client;
};

/// This class provides methods for launching internal clients.
///
/// \sa miral::ExternalClientLauncher - for launching clients as an external process
class InternalClientLauncher
{
public:
    InternalClientLauncher();
    ~InternalClientLauncher();

    void operator()(mir::Server& server);

    /// Launch an internal client in another thread.
    ///
    /// The \p connect_notification call will be called when a connection to the server has
    /// been secured. This is called on a worker thread and must not block.
    ///
    /// The \p wayland_fd callback will be called when the client has been initialized and is ready
    /// to start interacting with the server. Note that this callback happens on another thread.
    /// This callback must exit.
    ///
    /// \param wayland_fd called when the Wayland client is initialized
    /// \param connect_notification called when the session has connected
    void launch(
        std::function<void(struct ::wl_display* display)> const& wayland_fd,
        std::function<void(std::weak_ptr<mir::scene::Session> const session)> const& connect_notification) const;

    /// Launch an internal client in another read.
    ///
    /// The \p client_object instance must define:
    /// - `operator()(struct wl_display*)` - the method called when the client has been initialized
    ///                                      and is ready to start interacting with the server. Note that
    ///                                      this callback happens in another thread. This callback must exit.
    /// - `operator()(std::weak_ptr<mir::scene::Session> const)` - the method called when a connection to the server
    ///                                                            has been secured. This is called on a worker thread
    ///                                                            and must not block.
    ///
    /// \param client_object an object describing the client connection
    template <typename ClientObject>
    void launch(ClientObject& client_object) const
    {
        launch(
            [&](struct ::wl_display* display) { client_object(display); },
            [&](std::weak_ptr<mir::scene::Session> const session) { client_object(session); });
    }

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_INTERNAL_CLIENT_H
