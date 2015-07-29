/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_PROTOBUF_MESSAGE_SENDER_H_
#define MIR_FRONTEND_PROTOBUF_MESSAGE_SENDER_H_

#include "mir/frontend/fd_sets.h"
#include <google/protobuf/stubs/common.h>

namespace google { namespace protobuf { class MessageLite; } }
namespace mir
{
namespace frontend
{
namespace detail
{
class ProtobufMessageSender
{
public:
    virtual void send_response(
    google::protobuf::uint32 call_id,
    google::protobuf::MessageLite* message,
    FdSets const& fd_sets) = 0;

protected:
    ProtobufMessageSender() = default;
    virtual ~ProtobufMessageSender() = default;

private:
    ProtobufMessageSender(ProtobufMessageSender const&) = delete;
    ProtobufMessageSender& operator=(ProtobufMessageSender const&) = delete;
};
}
}
}

#endif /* MIR_FRONTEND_PROTOBUF_MESSAGE_SENDER_H_ */
