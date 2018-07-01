/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIRAL_INTERNAL_CLIENT_H
#define MIRAL_INTERNAL_CLIENT_H

#include <mir/client/connection.h>

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
class StartupInternalClient
{
public:
    explicit StartupInternalClient(
        std::string name,
        std::function<void(mir::client::Connection connection)> client_code,
        std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification);

    template <typename ClientObject>
    explicit StartupInternalClient(std::string name, ClientObject const& client_object) :
        StartupInternalClient(name, client_object, client_object) {}

    explicit StartupInternalClient(
        std::function<void(struct ::wl_display* display)> client_code,
        std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification);

    template <typename ClientObject>
    explicit StartupInternalClient(ClientObject const& client_object) :
        StartupInternalClient(client_object, client_object) {}

    ~StartupInternalClient();

    void operator()(mir::Server& server);

private:
    class Self;
    std::shared_ptr<Self> internal_client;
};

class InternalClientLauncher
{
public:
    InternalClientLauncher();
    ~InternalClientLauncher();

    void operator()(mir::Server& server);

    void launch(
        std::string const& name,
        std::function<void(mir::client::Connection connection)> const& client_code,
        std::function<void(std::weak_ptr<mir::scene::Session> const session)> const& connect_notification) const;

    template <typename ClientObject>
    void launch(std::string const& name, ClientObject& client_object) const
    {
        launch(
            name,
            [&](mir::client::Connection connection) { client_object(connection); },
            [&](std::weak_ptr<mir::scene::Session> const session) { client_object(session); });
    }

    void launch(
        std::function<void(struct ::wl_display* display)> const& wayland_fd,
        std::function<void(std::weak_ptr<mir::scene::Session> const session)> const& connect_notification) const;

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
