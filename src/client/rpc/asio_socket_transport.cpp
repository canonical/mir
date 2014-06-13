#include "asio_socket_transport.h"
#include "mir/variable_length_array.h"

#include <signal.h>

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>



namespace mclr = mir::client::rpc;

mclr::AsioSocketTransport::AsioSocketTransport(int /*fd*/)
    : work{io_service},
      socket{io_service}
{
}

mclr::AsioSocketTransport::AsioSocketTransport(std::string const& /*socket_path*/)
    : work{io_service},
      socket{io_service}
{
}

mclr::AsioSocketTransport::~AsioSocketTransport()
{
    io_service.stop();

    if (io_service_thread.joinable())
    {
        io_service_thread.join();
    }
}

void mclr::AsioSocketTransport::register_observer(std::shared_ptr<Observer> const&/*callback*/)
{
}

size_t mclr::AsioSocketTransport::receive_data(void* /*buffer*/, size_t /*message_size*/)
{
    return 0;
}

void mclr::AsioSocketTransport::receive_file_descriptors(std::vector<int> &fds)
{
    // We send dummy data
    struct iovec iov;
    char dummy_iov_data = '\0';
    iov.iov_base = &dummy_iov_data;
    iov.iov_len = 1;

    // Allocate space for control message
    static auto const builtin_n_fds = 5;
    static auto const builtin_cmsg_space = CMSG_SPACE(builtin_n_fds * sizeof(int));
    auto const fds_bytes = fds.size() * sizeof(int);
    mir::VariableLengthArray<builtin_cmsg_space> control{CMSG_SPACE(fds_bytes)};

    // Message to send
    struct msghdr header;
    header.msg_name = NULL;
    header.msg_namelen = 0;
    header.msg_iov = &iov;
    header.msg_iovlen = 1;
    header.msg_controllen = control.size();
    header.msg_control = control.data();
    header.msg_flags = 0;

    recvmsg(socket.native_handle(), &header, 0);

    // If we get a proper control message, copy the received
    // file descriptors back to the caller
    struct cmsghdr const* const cmsg = CMSG_FIRSTHDR(&header);

    if (cmsg && cmsg->cmsg_len == CMSG_LEN(fds_bytes) &&
        cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
    {
        int const* const data = reinterpret_cast<int const*>CMSG_DATA(cmsg);
        int i = 0;
        for (auto& fd : fds)
            fd = data[i++];
    }
    else
    {
        BOOST_THROW_EXCEPTION(
                    std::runtime_error("Invalid control message for receiving file descriptors"));
    }
}

size_t mclr::AsioSocketTransport::send_data(const std::vector<uint8_t>& buffer)
{
    boost::system::error_code error;

    boost::asio::write(
        socket,
        boost::asio::buffer(buffer),
        error);

    if (error)
    {
//        rpc_report->invocation_failed(invocation, error);
//        notify_disconnected();
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to send message to server: " + error.message()));
    }
    return 0;
}

void mclr::AsioSocketTransport::init()
{
    io_service_thread = std::thread([this]
        {
            // Our IO threads must not receive any signals
            sigset_t all_signals;
            sigfillset(&all_signals);

            if (auto error = pthread_sigmask(SIG_BLOCK, &all_signals, NULL))
                BOOST_THROW_EXCEPTION(
                    boost::enable_error_info(
                        std::runtime_error("Failed to block signals on IO thread")) << boost::errinfo_errno(error));
/*
            boost::asio::async_read(
                socket,
                boost::asio::buffer(header_bytes),
                boost::asio::transfer_exactly(sizeof header_bytes),
                boost::bind(&MirProtobufRpcChannel::on_header_read, this,
                    boost::asio::placeholders::error));
*/
            try
            {
                io_service.run();
            }
            catch (std::exception const& x)
            {
//                rpc_report->connection_failure(x);

//                notify_disconnected();
            }
        });
}
