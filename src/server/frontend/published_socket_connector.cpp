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

#include "published_socket_connector.h"
#include "protobuf_session_creator.h"

#include "mir/frontend/connector_report.h"

#include <boost/signals2.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <sys/socket.h>

namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;

mf::PublishedSocketConnector::PublishedSocketConnector(
    const std::string& socket_file,
    std::shared_ptr<SessionCreator> const& session_creator,
    int threads,
    std::function<void()> const& force_threads_to_unblock,
    std::shared_ptr<ConnectorReport> const& report)
:   BasicConnector(session_creator, threads, force_threads_to_unblock, report),
    socket_file(socket_file),
    acceptor(io_service, socket_file)
{
    start_accept();
}

mf::PublishedSocketConnector::~PublishedSocketConnector() noexcept
{
    remove_endpoint();
}

void mf::PublishedSocketConnector::start_accept()
{
    report->listening_on(socket_file);

    auto socket = std::make_shared<boost::asio::local::stream_protocol::socket>(io_service);

    acceptor.async_accept(
        *socket,
        boost::bind(
            &PublishedSocketConnector::on_new_connection,
            this,
            socket,
            ba::placeholders::error));
}

void mf::PublishedSocketConnector::remove_endpoint() const
{
    std::remove(socket_file.c_str());
}

void mf::PublishedSocketConnector::on_new_connection(
    std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket,
    boost::system::error_code const& ec)
{
    if (!ec)
    {
        create_session_for(socket);
    }
    start_accept();
}

mf::BasicConnector::BasicConnector(
    std::shared_ptr<SessionCreator> const& session_creator,
    int threads,
    std::function<void()> const& force_threads_to_unblock,
    std::shared_ptr<ConnectorReport> const& report)
:   report(report),
    io_service_threads(threads),
    force_threads_to_unblock(force_threads_to_unblock),
    session_creator{session_creator}
{
}

void mf::BasicConnector::start()
{
    auto run_io_service = [this]
    {
        while (true)
        try
        {
            report->thread_start();
            io_service.run();
            report->thread_end();
            return;
        }
        catch (std::exception const& e)
        {
            report->error(e);
        }
    };

    report->starting_threads(io_service_threads.size());
    for (auto& thread : io_service_threads)
    {
        thread = std::thread(run_io_service);
    }
}

void mf::BasicConnector::stop()
{
    /* Stop processing new requests */
    io_service.stop();

    /*
     * Ensure that any pending requests will complete (i.e., that they
     * will not block indefinitely waiting for a resource from the server)
     */
    force_threads_to_unblock();

    report->stopping_threads(io_service_threads.size());

    /* Wait for all io processing threads to finish */
    for (auto& thread : io_service_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    /* Prepare for a potential restart */
    io_service.reset();
}

void mf::BasicConnector::create_session_for(std::shared_ptr<boost::asio::local::stream_protocol::socket> const& server_socket) const
{
    report->creating_session_for(server_socket->native_handle());
    session_creator->create_session_for(server_socket);
}

int mf::BasicConnector::client_socket_fd() const
{
    enum { server, client, size };
    int socket_fd[size];

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socket_fd))
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Could not create socket pair")) << boost::errinfo_errno(errno));
    }

    auto const server_socket = std::make_shared<boost::asio::local::stream_protocol::socket>(
        io_service, boost::asio::local::stream_protocol(), socket_fd[server]);

    report->creating_socket_pair(socket_fd[server], socket_fd[client]);

    create_session_for(server_socket);

    return socket_fd[client];
}

void mf::BasicConnector::remove_endpoint() const
{
}

mf::BasicConnector::~BasicConnector() noexcept
{
    stop();
}

void mf::NullConnectorReport::thread_start()
{
}

void mf::NullConnectorReport::thread_end()
{
}

void mf::NullConnectorReport::starting_threads(int /*count*/)
{
}

void mf::NullConnectorReport::stopping_threads(int /*count*/)
{
}

void mf::NullConnectorReport::creating_session_for(int /*socket_handle*/)
{
}

void mf::NullConnectorReport::creating_socket_pair(int /*server_handle*/, int /*client_handle*/)
{
}

void mf::NullConnectorReport::listening_on(std::string const& /*endpoint*/)
{
}

void mf::NullConnectorReport::error(std::exception const& /*error*/)
{
}
