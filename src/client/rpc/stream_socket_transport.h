/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */


#ifndef STREAM_SOCKET_TRANSPORT_H_
#define STREAM_SOCKET_TRANSPORT_H_

#include "stream_transport.h"
#include "mir/fd.h"
#include "mir/basic_observers.h"

#include <thread>
#include <mutex>

namespace mir
{
namespace client
{
namespace rpc
{

class TransportObservers : public StreamTransport::Observer,
                           private BasicObservers<StreamTransport::Observer>
{
public:
    using BasicObservers<StreamTransport::Observer>::add;
    using BasicObservers<StreamTransport::Observer>::remove;

    void on_data_available() override;
    void on_disconnected() override;
};

class StreamSocketTransport : public StreamTransport
{
public:
    StreamSocketTransport(Fd const& fd);
    StreamSocketTransport(std::string const& socket_path);

    void register_observer(std::shared_ptr<Observer> const& observer) override;
    void unregister_observer(std::shared_ptr<Observer> const& observer) override;

    void receive_data(void* buffer, size_t bytes_requested) override;
    void receive_data(void* buffer, size_t bytes_requested, std::vector<Fd>& fds) override;
    void send_message(std::vector<uint8_t> const& buffer, std::vector<mir::Fd> const& fds) override;

    Fd watch_fd() const override;
    bool dispatch(mir::dispatch::FdEvents event) override;
    mir::dispatch::FdEvents relevant_events() const override;
private:
    Fd open_socket(std::string const& path);

    Fd const socket_fd;

    TransportObservers observers;
};

}
}
}


#endif // STREAM_SOCKET_TRANSPORT_H_
