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


#ifndef ASIO_SOCKET_TRANSPORT_H
#define ASIO_SOCKET_TRANSPORT_H

#include "stream_transport.h"

#include <thread>
#include <mutex>

#include <boost/asio.hpp>

namespace mir
{
namespace client
{
namespace rpc
{

class AsioSocketTransport : public StreamTransport
{
public:
    AsioSocketTransport(int fd);
    AsioSocketTransport(std::string const& socket_path);
    ~AsioSocketTransport() override;

    // Transport interface
public:
    void register_observer(std::shared_ptr<Observer> const& observer) override;
    void receive_data(void* buffer, size_t read_bytes) override;
    void receive_data(void* buffer, size_t read_bytes, std::vector<int>& fds) override;
    void send_data(const std::vector<uint8_t> &buffer) override;

private:
    void init();
    void on_data_available(boost::system::error_code const& ec, size_t /*bytes_read*/);
    void notify_data_available();
    void notify_disconnected();

    std::thread io_service_thread;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work;
    std::mutex observer_mutex;
    boost::asio::local::stream_protocol::socket socket;
    std::vector<std::shared_ptr<Observer>> observers;
};

}
}
}


#endif // ASIO_SOCKET_TRANSPORT_H
