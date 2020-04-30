/*
 * Copyright © 2016-2020 Canonical Ltd.
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
#include <mir/main_loop.h>
#include <mir/server.h>
#include <mir/scene/session.h>
#include <mir/raii.h>

#define MIR_LOG_COMPONENT "miral::Internal Client"
#include <mir/log.h>

#include <wayland-client.h>

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
    virtual void run(mir::Server& server) = 0;

    virtual void join_client_thread() = 0;

    virtual ~InternalClientRunner() = default;

    InternalClientRunner() = default;
    InternalClientRunner(InternalClientRunner const&) = delete;
    InternalClientRunner& operator=(InternalClientRunner const&) = delete;
};
}

class miral::StartupInternalClient::Self : public virtual InternalClientRunner
{
};

namespace
{
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

// "Base" is a bit of compile time indirection because StartupInternalClient::Self is inaccessible
template<typename Base>
class WlInternalClientRunner : public Base, public virtual InternalClientRunner
{
public:
    WlInternalClientRunner(
        std::function<void(struct ::wl_display* display)> client_code,
        std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification);

    void run(mir::Server& server) override;
    void join_client_thread() override;

    ~WlInternalClientRunner() override;

private:
    std::thread thread;
    std::function<void(struct ::wl_display* display)> const client_code;
    std::function<void(std::weak_ptr<mir::scene::Session> const session)> const connect_notification;
};

template<typename Base>
WlInternalClientRunner<Base>::WlInternalClientRunner(
    std::function<void(struct ::wl_display* display)> client_code,
    std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification) :
    client_code(std::move(client_code)),
    connect_notification(std::move(connect_notification))
{
}

template<typename Base>
void WlInternalClientRunner<Base>::run(mir::Server& server)
{
    int fd = server.open_client_wayland([this](std::shared_ptr<mir::scene::Session> const& mf_session)
        {
            connect_notification(std::dynamic_pointer_cast<mir::scene::Session>(mf_session));
        });

    thread = std::thread{[this, fd]
        {
            try
            {
                if (auto const display = wl_display_connect_to_fd(fd))
                {
                    auto const deleter = mir::raii::deleter_for(display, &wl_display_disconnect);
                    client_code(display);
                    wl_display_roundtrip(display);
                }
            }
            catch (std::exception const& e)
            {
                mir::log(mir::logging::Severity::informational, MIR_LOG_COMPONENT,
                         std::make_exception_ptr(e), e.what());
            }
        }};
}

template<typename Base>
WlInternalClientRunner<Base>::~WlInternalClientRunner()
{
    join_client_thread();
}


template<typename Base>
void WlInternalClientRunner<Base>::join_client_thread()
{
    if (thread.joinable())
    {
        thread.join();
    }
}

miral::StartupInternalClient::StartupInternalClient(
    std::function<void(struct ::wl_display* display)> client_code,
    std::function<void(std::weak_ptr<mir::scene::Session> const session)> connect_notification) :
    internal_client(std::make_shared<WlInternalClientRunner<Self>>(std::move(client_code), std::move(connect_notification)))
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
    std::function<void(struct ::wl_display* display)> const& client_code,
    std::function<void(std::weak_ptr<mir::scene::Session> const session)> const& connect_notification) const
{
    self->runner = std::make_shared<WlInternalClientRunner<Self>>(client_code, connect_notification);
    self->server->the_main_loop()->enqueue(this, [this] { self->runner->run(*self->server); });
    register_runner(self->server, self->runner);
}

miral::InternalClientLauncher::InternalClientLauncher() : self{std::make_shared<Self>()} {}
miral::InternalClientLauncher::~InternalClientLauncher() = default;

void join_client_threads(mir::Server* server)
{
    join_runners_for(server);
}
