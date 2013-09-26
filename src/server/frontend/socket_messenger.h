/*
 * Copyright © 2013 Canonical Ltd.
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

    void send(std::string const& body);
    void send(std::string const& body, FdSets const& fd_set);

    void async_receive_msg(MirReadHandler const& handler, boost::asio::streambuf& buffer, size_t size); 
    pid_t client_pid();

private:
    std::shared_ptr<boost::asio::local::stream_protocol::socket> socket;
    
    std::mutex message_lock;
    std::vector<char> whole_message;
    
    void send_fds_locked(std::unique_lock<std::mutex> const& lock, std::vector<int32_t> const& fds);
};
}
}
}

#endif /* MIR_FRONTEND_SOCKET_MESSENGER_H_ */ 
