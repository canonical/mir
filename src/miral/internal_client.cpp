/*
 * Copyright © 2016-2017 Canonical Ltd.
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

#include "miral/internal_client.h"
#include "join_client_threads.h"

#include <mir/fd.h>
#include <mir/server.h>
#include <mir/scene/session.h>
#include <mir/main_loop.h>

#define MIR_LOG_COMPONENT "miral::Internal Client"
#include <mir/log.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <map>

namespace
{
class InternalClientRunner
{
public:
    InternalClientRunner(std::string name,
         std::function<void(mir::client::Connection connection)> client_code,
         std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification);

    void run(mir::Server& server);
    void join_client_thread();

    ~InternalClientRunner();

private:
    std::string const name;
    std::thread thread;
    std::mutex mutable mutex;
    std::condition_variable mutable cv;
    mir::Fd fd;
    std::weak_ptr<mir::scene::Session> session;
    mir::client::Connection connection;
    std::function<void(mir::client::Connection connection)> const client_code;
    std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification;
};

std::mutex client_runners_mutex;
std::multimap<mir::Server*, std::weak_ptr<InternalClientRunner>> client_runners;

void register_runner(mir::Server* server, std::weak_ptr<InternalClientRunner> internal_client)
{
    std::lock_guard<decltype(client_runners_mutex)> lock{client_runners_mutex};
    client_runners.emplace(server, std::move(internal_client));
}

void join_runners_for(mir::Server* server)
{
    std::lock_guard<decltype(client_runners_mutex)> lock{client_runners_mutex};
    auto range = client_runners.equal_range(server);

    for (auto i = range.first; i != range.second; ++i)
    {
        if (auto runner = i->second.lock())
            runner->join_client_thread();
    }

    client_runners.erase(range.first, range.second);
}
}

class miral::StartupInternalClient::Self : public InternalClientRunner
{
    using InternalClientRunner::InternalClientRunner;
};

InternalClientRunner::InternalClientRunner(
    std::string const name,
    std::function<void(mir::client::Connection connection)> client_code,
    std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification) :
    name(name),
    client_code(std::move(client_code)),
    connect_notification(std::move(connect_notification))
{
}

void InternalClientRunner::run(mir::Server& server)
{
    fd = server.open_client_socket([this](std::shared_ptr<mir::frontend::Session> const& mf_session)
        {
            std::lock_guard<decltype(mutex)> lock_guard{mutex};
            session = std::dynamic_pointer_cast<mir::scene::Session>(mf_session);
            connect_notification(session);
            cv.notify_one();
        });

    char connect_string[64] = {0};
    sprintf(connect_string, "fd://%d", fd.operator int());

    connection = mir::client::Connection{mir_connect_sync(connect_string, name.c_str())};

    mir_connection_set_lifecycle_event_callback(
        connection,
        [](MirConnection*, MirLifecycleState transition, void*)
        {
            // The default handler kills the process with SIGHUP - which is unhelpful for internal clients
            if (transition == mir_lifecycle_connection_lost)
                mir::log_warning("server disconnected before connection released by client");
        },
        this);

    std::unique_lock<decltype(mutex)> lock{mutex};
    cv.wait(lock, [&] { return !!session.lock(); });

    thread = std::thread{[this]
        {
            client_code(connection);
            connection.reset();
        }};
}

InternalClientRunner::~InternalClientRunner()
{
    join_client_thread();
}


void InternalClientRunner::join_client_thread()
{
    if (thread.joinable())
    {
        thread.join();
    }
}

miral::StartupInternalClient::StartupInternalClient(
    std::string name,
    std::function<void(mir::client::Connection connection)> client_code,
    std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification) :
    internal_client(std::make_shared<Self>(std::move(name), std::move(client_code), std::move(connect_notification)))
{
}

void miral::StartupInternalClient::operator()(mir::Server& server)
{
    register_runner(&server, internal_client);

    server.add_init_callback([this, &server]
    {
        server.the_main_loop()->enqueue(this, [this, &server]
        {
            internal_client->run(server);
        });
    });
}

miral::StartupInternalClient::~StartupInternalClient() = default;

struct miral::InternalClientLauncher::Self
{
    mir::Server* server = nullptr;
    std::shared_ptr<InternalClientRunner> runner;
};

void miral::InternalClientLauncher::operator()(mir::Server& server)
{
    self->server = &server;
}

void miral::InternalClientLauncher::launch(
    std::string const& name,
    std::function<void(mir::client::Connection connection)> const& client_code,
    std::function<void(std::weak_ptr<mir::scene::Session> const session)> const& connect_notification) const
{
    self->runner = std::make_shared<InternalClientRunner>(name, client_code, connect_notification);
    self->server->the_main_loop()->enqueue(this, [this] { self->runner->run(*self->server); });
    register_runner(self->server, self->runner);
}

miral::InternalClientLauncher::InternalClientLauncher() : self{std::make_shared<Self>()} {}
miral::InternalClientLauncher::~InternalClientLauncher() = default;

void join_client_threads(mir::Server* server)
{
    join_runners_for(server);
}
