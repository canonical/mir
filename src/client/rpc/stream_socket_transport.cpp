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

#include "stream_socket_transport.h"
#include "mir/variable_length_array.h"
#include "mir/thread_name.h"
#include "mir/fd_socket_transmission.h"

#include <system_error>

#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace mclr = mir::client::rpc;

mclr::StreamSocketTransport::StreamSocketTransport(mir::Fd const& fd)
    : socket_fd{fd}
{
    init();
}

mclr::StreamSocketTransport::StreamSocketTransport(std::string const& socket_path)
    : StreamSocketTransport(open_socket(socket_path))
{
}

mclr::StreamSocketTransport::~StreamSocketTransport()
{
    int dummy{0};
    send(shutdown_fd, &dummy, sizeof(dummy), MSG_NOSIGNAL);
    if (io_service_thread.joinable())
    {
        io_service_thread.join();
    }
    close(shutdown_fd);
}

void mclr::StreamSocketTransport::register_observer(std::shared_ptr<Observer> const& observer)
{
    std::lock_guard<decltype(observer_mutex)> lock(observer_mutex);
    observers.push_back(observer);
}

void mclr::StreamSocketTransport::receive_data(void* buffer, size_t bytes_requested)
{
    /*
     * Explicitly implement this, rather than delegating to receive_data(buffer, size, fds)
     * so that we can catch when we discard file descriptors.
     *
     * See comment for DISABLED_ReceivingMoreFdsThanExpectedInMultipleChunksRaisesException
     * test in test_stream_transport.cpp for details.
     */
    if (bytes_requested == 0)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Attempted to receive 0 bytes"));
    }
    size_t bytes_read{0};
    while(bytes_read < bytes_requested)
    {
        // Store the data in the buffer requested
        struct iovec iov;
        iov.iov_base = static_cast<uint8_t*>(buffer) + bytes_read;
        iov.iov_len = bytes_requested - bytes_read;

        // Message to read
        struct msghdr header;
        header.msg_name = NULL;
        header.msg_namelen = 0;
        header.msg_iov = &iov;
        header.msg_iovlen = 1;
        header.msg_controllen = 0;
        header.msg_control = nullptr;
        header.msg_flags = 0;

        ssize_t const result = recvmsg(socket_fd, &header, MSG_NOSIGNAL | MSG_WAITALL);

        if (result == 0)
        {
            notify_disconnected();
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to read message from server: server has shutdown"));
        }
        if (result < 0)
        {
            if (socket_error_is_transient(errno))
            {
                continue;
            }
            if (errno == EPIPE)
            {
                notify_disconnected();
                BOOST_THROW_EXCEPTION(
                            boost::enable_error_info(
                                socket_disconnected_error("Failed to read message from server"))
                            << boost::errinfo_errno(errno));
            }
            BOOST_THROW_EXCEPTION(
                        boost::enable_error_info(socket_error("Failed to read message from server"))
                             << boost::errinfo_errno(errno));
        }

        if (header.msg_flags & MSG_CTRUNC)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Unexpectedly received fds"));
        }

        bytes_read += result;
    }
}

void mclr::StreamSocketTransport::receive_data(void* buffer, size_t bytes_requested, std::vector<mir::Fd>& fds)
try
{
    mir::receive_data(socket_fd, buffer, bytes_requested, fds);
}
catch (socket_disconnected_error &e)
{
    notify_disconnected();
    throw e;
}

void mclr::StreamSocketTransport::send_message(
    std::vector<uint8_t> const& buffer,
    std::vector<mir::Fd> const& fds)
{
    size_t bytes_written{0};
    while (bytes_written < buffer.size())
    {
        ssize_t const result = send(socket_fd,
                                    buffer.data() + bytes_written,
                                    buffer.size() - bytes_written,
                                    MSG_NOSIGNAL);

        if (result < 0)
        {
            if (socket_error_is_transient(errno))
            {
                continue;
            }
            if (errno == EPIPE)
            {
                notify_disconnected();
                BOOST_THROW_EXCEPTION(
                            boost::enable_error_info(socket_disconnected_error("Failed to send message to server"))
                            << boost::errinfo_errno(errno));
            }
            BOOST_THROW_EXCEPTION(
                        boost::enable_error_info(socket_error("Failed to send message to server"))
                        << boost::errinfo_errno(errno));
        }
        bytes_written += result;
    }

    if (!fds.empty())
        mir::send_fds(socket_fd, fds);
}

void mclr::StreamSocketTransport::init()
{
    // We use sockets rather than a pipe so that we can control
    // EPIPE behaviour; we don't want SIGPIPE when the IO loop terminates.
    int socket_fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    this->shutdown_fd = mir::Fd{socket_fds[1]};

    auto shutdown_fd = mir::Fd{socket_fds[0]};
    io_service_thread = std::thread([this, shutdown_fd]
    {
        // Our IO threads must not receive any signals
        sigset_t all_signals;
        sigfillset(&all_signals);

        if (auto error = pthread_sigmask(SIG_BLOCK, &all_signals, NULL))
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error("Failed to block signals on IO thread")) << boost::errinfo_errno(error));

        mir::set_thread_name("Client IO loop");

        int epoll_fd = epoll_create1(0);

        epoll_event event;
        // Make valgrind happy, harder
        memset(&event, 0, sizeof(event));

        event.events = EPOLLIN | EPOLLRDHUP;
        event.data.fd = socket_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event);

        event.events = EPOLLIN | EPOLLRDHUP;
        event.data.fd = shutdown_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, shutdown_fd, &event);

        bool shutdown_requested{false};
        while (!shutdown_requested)
        {
            epoll_event event;
            epoll_wait(epoll_fd, &event, 1, -1);
            if (event.data.fd == socket_fd)
            {
                if (event.events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                {
                    if (event.events & EPOLLIN)
                    {
                        // If the remote end shut down cleanly it's possible there's some more
                        // data left to read, or that reads will now return 0 (EOF)
                        //
                        // If there's more data left to read, notify of this before disconnect.
                        int dummy;
                        if (recv(socket_fd, &dummy, sizeof(dummy), MSG_PEEK | MSG_NOSIGNAL) > 0)
                        {
                            try
                            {
                                notify_data_available();
                            }
                            catch(...)
                            {
                                //It's quite likely that notify_data_available() will lead to
                                //an exception being thrown; after all, the remote has closed
                                //the connection.
                                //
                                //This doesn't matter; we're already shutting down.
                            }
                        }
                    }
                    notify_disconnected();
                    shutdown_requested = true;
                }
                else if (event.events & EPOLLIN)
                {
                    try
                    {
                        notify_data_available();
                    }
                    catch (socket_disconnected_error &err)
                    {
                        // We've already notified of disconnection.
                        shutdown_requested = true;
                    }
                    // These need not be fatal.
                    catch (fd_reception_error &err)
                    {
                    }
                    catch (socket_error &err)
                    {
                    }
                    catch (...)
                    {
                        // We've no idea what the problem is, so clean up as best we can.
                        notify_disconnected();
                        shutdown_requested = true;
                    }
                }
            }
            if (event.data.fd == shutdown_fd)
            {
                shutdown_requested = true;
            }
        }
        ::close(epoll_fd);
    });
}

mir::Fd mclr::StreamSocketTransport::open_socket(std::string const& path)
{
    struct sockaddr_un socket_address;
    // Appease the almighty valgrind
    memset(&socket_address, 0, sizeof(socket_address));

    socket_address.sun_family = AF_UNIX;
    // Must be memcpy rather than strcpy, as abstract socket paths start with '\0'
    memcpy(socket_address.sun_path, path.data(), path.size());

    mir::Fd fd{socket(AF_UNIX, SOCK_STREAM, 0)};
    if (connect(fd, reinterpret_cast<sockaddr*>(&socket_address), sizeof(socket_address)) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(socket_error("Failed to connect to server socket"))
                    << boost::errinfo_errno(errno));
    }
    return fd;
}

void mclr::StreamSocketTransport::notify_data_available()
{
    // TODO: If copying the observers turns out to be slow, replace with
    // an RCUish data type; this is a read-mostly, write infrequently structure.
    decltype(observers) observer_copy;
    {
        std::lock_guard<decltype(observer_mutex)> lock(observer_mutex);
        observer_copy = observers;
    }
    for (auto& observer : observer_copy)
    {
        observer->on_data_available();
    }
}

void mclr::StreamSocketTransport::notify_disconnected()
{
    decltype(observers) observer_copy;
    {
        std::lock_guard<decltype(observer_mutex)> lock(observer_mutex);
        observer_copy = observers;
    }
    for (auto& observer : observer_copy)
    {
        observer->on_disconnected();
    }
}
