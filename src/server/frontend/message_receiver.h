/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com> 
 */
#ifndef MIR_FRONTEND_MESSAGE_RECEIVER_H_
#define MIR_FRONTEND_MESSAGE_RECEIVER_H_

#include <functional>
#include <boost/asio.hpp>

namespace mir
{
namespace frontend
{
namespace detail
{
class MessageReceiver
{
public:
    //receive message from the socket. 'handler' will be called when 'buffer' has been filled with exactly 'size'
    typedef std::function<void(boost::system::error_code const&, size_t)> MirReadHandler;
    virtual void async_receive_msg(MirReadHandler const& handler, boost::asio::streambuf& buffer, size_t size) = 0; 
    virtual pid_t client_pid() = 0;

protected:
    MessageReceiver() = default;
    virtual ~MessageReceiver() = default;
    MessageReceiver(MessageReceiver const&) = delete;
    MessageReceiver& operator=(MessageReceiver const&) = delete;
};

}
}
}
#endif /* MIR_FRONTEND_MESSAGE_RECEIVER_H_ */
