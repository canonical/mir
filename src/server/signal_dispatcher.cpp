/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/signal_dispatcher.h"

#include <mutex>
#include <thread>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>


namespace
{

struct EventSocketPair
{
    typedef unsigned int SocketIdentifier;

    static const SocketIdentifier socket_a = 0;
    static const SocketIdentifier socket_b = 1;

    EventSocketPair()
    {
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
            throw std::runtime_error("Couldn't create socketpair");
    }

    ~EventSocketPair()
    {
        ::close(sockets[0]);
        ::close(sockets[1]);
    }

    void write_signal_to(SocketIdentifier id, int signal)
    {
        if (::write(sockets[id], &signal, sizeof(signal)) < 0)
        {
            // TODO: We cannot throw an exception in code that is
            // called from a signal handler. Reenable once we have
            // signal-safe logging in place.
            // throw std::runtime_error("Problem writing to socket");
        }
    }

    int read_signal_from(SocketIdentifier id)
    {
        int signal{0};
        ssize_t nread{0};

        while (nread <= 0)
        {
            nread = ::read(sockets[id], &signal, sizeof(signal));

            if (nread == 0)
                throw std::runtime_error("Write end of socket has been closed");
            else if (nread > 0 && nread < static_cast<ssize_t>(sizeof(signal)))
                throw std::runtime_error("Incomplete message retrieved");
            else if (nread < 0 && errno != EINTR)
                throw std::runtime_error("Problem reading from socket");
        }

        return signal;
    }

    int sockets[2];
} event_socket_pair;

}

struct mir::SignalDispatcher::Constructor
{
    static SignalDispatcher* construct()
    {
        return new SignalDispatcher();
    }
};

namespace
{
std::shared_ptr<mir::SignalDispatcher> instance;
std::once_flag init_flag;

void init()
{
    instance.reset(mir::SignalDispatcher::Constructor::construct());
}

void signal_handler(int signal)
{
    event_socket_pair.write_signal_to(
        EventSocketPair::socket_a,
        signal);
}

}

struct mir::SignalDispatcher::Private
{
    Private() : worker_thread(std::bind(&Private::worker, this))
    {
        worker_thread.detach();
    }

    void worker()
    {
        while(true)
        {
            // Todo: Use polling here.
            try
            {
                int s = event_socket_pair.read_signal_from(
                    EventSocketPair::socket_b);
                signal_channel(s);
            } catch(...)
            {
                break;
            }
        }
    }

    std::thread worker_thread;
    mir::SignalDispatcher::SignalType signal_channel;
};

std::shared_ptr<mir::SignalDispatcher> mir::SignalDispatcher::instance()
{
    std::call_once(::init_flag, ::init);

    return ::instance;
}

mir::SignalDispatcher::SignalDispatcher() : p(new Private())
{
}

mir::SignalDispatcher::~SignalDispatcher()
{
}

mir::SignalDispatcher::SignalType& mir::SignalDispatcher::signal_channel()
{
    return p->signal_channel;
}

void mir::SignalDispatcher::enable_for(int signal)
{
    struct sigaction action;
    ::memset(&action, 0, sizeof(action));
    ::sigemptyset(&action.sa_mask);
    ::sigaddset(&action.sa_mask, signal);
    action.sa_handler = ::signal_handler;
    action.sa_flags = SA_RESTART | SA_NODEFER;
    ::sigaction(signal, &action, NULL);
}
