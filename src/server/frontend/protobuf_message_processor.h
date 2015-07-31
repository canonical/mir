/*
 * Copyright © 2012. 2014 Canonical Ltd.
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


#ifndef MIR_FRONTEND_PROTOBUF_MESSAGE_PROCESSOR_H_
#define MIR_FRONTEND_PROTOBUF_MESSAGE_PROCESSOR_H_

#include "mir/frontend/message_processor.h"
#include "mir_protobuf.pb.h"
#include <google/protobuf/stubs/common.h>

#include <memory>

namespace google { namespace protobuf { class MessageLite; } }
namespace mir
{
namespace frontend
{
class MessageProcessorReport;

namespace detail
{
class DisplayServer;
class ProtobufMessageSender;

class ProtobufMessageProcessor : public MessageProcessor,
                                 public std::enable_shared_from_this<ProtobufMessageProcessor>
{
public:
    ProtobufMessageProcessor(
        std::shared_ptr<ProtobufMessageSender> const& sender,
        std::shared_ptr<DisplayServer> const& display_server,
        std::shared_ptr<MessageProcessorReport> const& report);

    ~ProtobufMessageProcessor() noexcept {}

    void client_pid(int pid) override;

    void send_response(google::protobuf::uint32 id, google::protobuf::MessageLite* response);
    void send_response(google::protobuf::uint32 id, protobuf::Buffer* response);
    void send_response(google::protobuf::uint32 id, protobuf::Connection* response);
    void send_response(google::protobuf::uint32 id, protobuf::Surface* response);
    void send_response(google::protobuf::uint32 id, std::shared_ptr<protobuf::Buffer> response);
    void send_response(google::protobuf::uint32 id, mir::protobuf::Screencast* response);
    void send_response(google::protobuf::uint32 id, mir::protobuf::BufferStream* response);
    void send_response(google::protobuf::uint32 id, mir::protobuf::SocketFD* response);
    void send_response(google::protobuf::uint32 id, std::shared_ptr<protobuf::PlatformOperationMessage> response);

private:
    bool dispatch(Invocation const& invocation, std::vector<mir::Fd> const& side_channel_fds) override;

    std::shared_ptr<ProtobufMessageSender> const sender;
    std::shared_ptr<DisplayServer> const display_server;
    std::shared_ptr<MessageProcessorReport> const report;
};
}
}
}

#endif /* PROTOBUF_MESSAGE_PROCESSOR_H_ */
