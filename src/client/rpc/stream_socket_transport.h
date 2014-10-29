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

#include <thread>
#include <mutex>

namespace mir
{
namespace client
{
namespace rpc
{

class StreamSocketTransport : public StreamTransport
{
public:
    StreamSocketTransport(Fd const& fd);
    StreamSocketTransport(std::string const& socket_path);
    ~StreamSocketTransport() override;

    void register_observer(std::shared_ptr<Observer> const& observer) override;
    void receive_data(void* buffer, size_t bytes_requested) override;
    void receive_data(void* buffer, size_t bytes_requested, std::vector<Fd>& fds) override;
    void send_message(std::vector<uint8_t> const& buffer, std::vector<mir::Fd> const& fds) override;

private:
    void init();
    Fd open_socket(std::string const& path);
    void notify_data_available();
    void notify_disconnected();

    std::thread io_service_thread;
    Fd const socket_fd;
    Fd shutdown_fd;

    std::mutex observer_mutex;
    std::vector<std::shared_ptr<Observer>> observers;
};

}
}
}


#endif // STREAM_SOCKET_TRANSPORT_H_
