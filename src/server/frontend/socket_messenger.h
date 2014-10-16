/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_FRONTEND_SOCKET_MESSENGER_H_
#define MIR_FRONTEND_SOCKET_MESSENGER_H_
#include "message_sender.h"
#include "message_receiver.h"
#include "mir/frontend/session_credentials.h"
#include <mutex>

namespace mir
{
namespace frontend
{
namespace detail
{
class SocketMessenger : public MessageSender,
                        public MessageReceiver
{
public:
    SocketMessenger(std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket);

    void send(char const* data, size_t length, FdSets const& fds) override;

    void async_receive_msg(MirReadHandler const& handler, boost::asio::mutable_buffers_1 const& buffer) override;
    boost::system::error_code receive_msg(boost::asio::mutable_buffers_1 const& buffer) override;
    size_t available_bytes() override;
    SessionCredentials client_creds() override;

private:
    void set_passcred(int opt);
    void update_session_creds();
    SessionCredentials creator_creds() const;

    std::shared_ptr<boost::asio::local::stream_protocol::socket> socket;
    mir::Fd socket_fd;

    std::mutex message_lock;
    SessionCredentials session_creds{0, 0, 0};

    void send_fds_locked(std::unique_lock<std::mutex> const& lock, std::vector<Fd> const& fds);
};
}
}
}

#endif /* MIR_FRONTEND_SOCKET_MESSENGER_H_ */
