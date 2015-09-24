/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_REORDERING_MESSAGE_SENDER_H_
#define MIR_FRONTEND_REORDERING_MESSAGE_SENDER_H_

#include "message_sender.h"

#include <mutex>

namespace mir
{
namespace frontend
{

/**
 * A MessageSender that buffers all messages until triggered,
 * then forwards all messages to an underlying MessageSender
 */
class ReorderingMessageSender : public MessageSender
{
public:
    explicit ReorderingMessageSender(std::shared_ptr<MessageSender> const& sink);

    void send(char const* data, size_t length, FdSets const& fds) override;

    /**
     * Stop diverting messages into the buffer.
     *
     * All messages sent prior to uncork() will be sent to the underlying MessageSender,
     * and all subsequent messages will be sent directly to the underlying MessageSender.
     */
    void uncork();
private:
    struct Message
    {
        std::vector<char> data;
        FdSets fds;
    };
    std::mutex message_lock;
    bool corked;
    std::vector<Message> buffered_messages;
    std::shared_ptr<MessageSender> const sink;
};

}
}

#endif //MIR_FRONTEND_REORDERING_MESSAGE_SENDER_H_
