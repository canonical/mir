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

#include "reordering_message_sender.h"

namespace mf = mir::frontend;

mf::ReorderingMessageSender::ReorderingMessageSender(std::shared_ptr<MessageSender> const& sink)
    : corked{true},
      sink{sink}
{
}

void mf::ReorderingMessageSender::send(
    char const* data,
    size_t length,
    mf::FdSets const& fds)
{
    {
        std::lock_guard<decltype(message_lock)> lock{message_lock};
        if (corked)
        {
            buffered_messages.emplace_back(Message {std::vector<char>(data, data + length), FdSets(fds)});
            return;
        }
    }

    sink->send(data, length, fds);
}

void mf::ReorderingMessageSender::uncork()
{
    {
        std::lock_guard<decltype(message_lock)> lock{message_lock};
        corked = false;
    }

    for (auto const& message : buffered_messages)
    {
        sink->send(message.data.data(), message.data.size(), message.fds);
    }
    buffered_messages.clear();
}
