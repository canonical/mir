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

#include "transport.h"

#include <thread>

#include <boost/asio.hpp>

namespace mir
{
namespace client
{
namespace rpc
{

class AsioSocketTransport : public Transport
{
public:
    AsioSocketTransport(int fd);
    AsioSocketTransport(std::string const& socket_path);
    ~AsioSocketTransport() override;

    // Transport interface
public:
    void register_observer(std::shared_ptr<Observer> const& observer) override;
    size_t receive_data(void* buffer, size_t message_size) override;
    void receive_file_descriptors(std::vector<int> &fds) override;
    size_t send_data(const std::vector<uint8_t> &buffer) override;

private:
    void init();

    std::thread io_service_thread;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work;
    boost::asio::local::stream_protocol::socket socket;
};

}
}
}


#endif // ASIO_SOCKET_TRANSPORT_H
