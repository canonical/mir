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

#include "mir/process/signal_dispatcher.h"

#include <boost/thread.hpp>

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace mp = mir::process;

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
        int signal;
        if (::read(sockets[id], &signal, sizeof(signal)) < 0)
            throw std::runtime_error("Problem reading_from socket");

        return signal;
    }
    
    int sockets[2];
} event_socket_pair;

}

struct mp::SignalDispatcher::Constructor
{
    static SignalDispatcher* construct()
    {
        return new SignalDispatcher();
    }
};

namespace global
{
std::shared_ptr<mp::SignalDispatcher> instance;
boost::once_flag init_flag;

void init()
{
    instance.reset(mp::SignalDispatcher::Constructor::construct());
}

void signal_handler(int signal)
{
    event_socket_pair.write_signal_to(
        EventSocketPair::socket_a,
        signal);
}

}

struct mp::SignalDispatcher::Private
{
    Private() : worker_thread(std::bind(&Private::worker, this))
    {
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
    
    boost::thread worker_thread;
    mp::SignalDispatcher::SignalType signal_channel;
};

std::shared_ptr<mp::SignalDispatcher> mp::SignalDispatcher::instance()
{
    boost::call_once(global::init_flag, global::init);

    return global::instance;
}

mp::SignalDispatcher::SignalDispatcher() : p(new Private())
{
}

mp::SignalDispatcher::~SignalDispatcher()
{
}
    
mp::SignalDispatcher::SignalType& mp::SignalDispatcher::signal_channel()
{
    return p->signal_channel;
}

void mp::SignalDispatcher::enable_for(int signal)
{
    struct sigaction action;
    ::memset(&action, 0, sizeof(action));
    ::sigemptyset(&action.sa_mask);
    ::sigaddset(&action.sa_mask, signal);
    action.sa_handler = global::signal_handler;
    action.sa_flags = SA_RESTART | SA_NODEFER;
    ::sigaction(signal, &action, NULL);
}
