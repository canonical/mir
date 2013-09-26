/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#ifndef MIR_FRONTEND_PROTOBUF_ASIO_COMMUNICATOR_H_
#define MIR_FRONTEND_PROTOBUF_ASIO_COMMUNICATOR_H_

#include "mir/frontend/connector.h"

#include <boost/asio.hpp>

#include <thread>
#include <string>
#include <vector>
#include <functional>

namespace google
{
namespace protobuf
{
class Message;
}
}

namespace mir
{
namespace frontend
{
class SessionCreator;
class ConnectorReport;

/// provides a client-side socket fd for each connection
class BasicConnector : public Connector
{
public:
    explicit BasicConnector(
        std::shared_ptr<SessionCreator> const& session_creator,
        int threads,
        std::function<void()> const& force_requests_to_complete,
        std::shared_ptr<ConnectorReport> const& report);
    ~BasicConnector() noexcept;
    void start() override;
    void stop() override;
    int client_socket_fd() const override;

protected:
    void create_session_for(std::shared_ptr<boost::asio::local::stream_protocol::socket> const& server_socket) const;
    boost::asio::io_service mutable io_service;

private:
    std::vector<std::thread> io_service_threads;
    std::function<void()> const force_requests_to_complete;
    std::shared_ptr<ConnectorReport> const report;
    std::shared_ptr<SessionCreator> const session_creator;
};

/// Accept connections over a published socket
class PublishedSocketConnector : public BasicConnector
{
public:
    explicit PublishedSocketConnector(
        const std::string& socket_file,
        std::shared_ptr<SessionCreator> const& session_creator,
        int threads,
        std::function<void()> const& force_requests_to_complete,
        std::shared_ptr<ConnectorReport> const& report);
    ~PublishedSocketConnector() noexcept;

private:
    void start_accept();
    void on_new_connection(std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket,
                           boost::system::error_code const& ec);

    const std::string socket_file;
    boost::asio::local::stream_protocol::acceptor acceptor;
};
}
}

#endif // MIR_FRONTEND_PROTOBUF_ASIO_COMMUNICATOR_H_
