/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_CLIENT_RPC_MIR_SOCKET_RPC_CHANNEL_H_
#define MIR_CLIENT_RPC_MIR_SOCKET_RPC_CHANNEL_H_

#include "mir_basic_rpc_channel.h"
#include "transport.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include <thread>

namespace mir
{
namespace protobuf
{
namespace wire
{
class Invocation;
class Result;
}
}

namespace client
{
class DisplayConfiguration;
class SurfaceMap;
class LifecycleControl;
class EventSink;
namespace rpc
{

class RpcReport;

class MirProtobufRpcChannel :
        public MirBasicRpcChannel,
        public StreamTransport::Observer
{
public:
    MirProtobufRpcChannel(std::unique_ptr<StreamTransport> transport,
                          std::shared_ptr<SurfaceMap> const& surface_map,
                          std::shared_ptr<DisplayConfiguration> const& disp_config,
                          std::shared_ptr<RpcReport> const& rpc_report,
                          std::shared_ptr<LifecycleControl> const& lifecycle_control,
                          std::shared_ptr<EventSink> const& event_sink);

    ~MirProtobufRpcChannel();

    void on_data_available() override;
    void on_disconnected() override;
private:
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method, google::protobuf::RpcController*,
        const google::protobuf::Message* parameters, google::protobuf::Message* response,
        google::protobuf::Closure* complete);

    std::shared_ptr<RpcReport> const rpc_report;
    detail::PendingCallCache pending_calls;

    static constexpr size_t size_of_header = 2;
    detail::SendBuffer header_bytes;
    detail::SendBuffer body_bytes;

    void receive_file_descriptors(google::protobuf::Message* response, google::protobuf::Closure* complete);
    template<class MessageType>
    void receive_any_file_descriptors_for(MessageType* response);
    void send_message(mir::protobuf::wire::Invocation const& body,
                      mir::protobuf::wire::Invocation const& invocation);

    void read_message();
    void process_event_sequence(std::string const& event);

    void notify_disconnected();

    std::unique_ptr<StreamTransport> transport;
    std::shared_ptr<SurfaceMap> surface_map;
    std::shared_ptr<DisplayConfiguration> display_configuration;
    std::shared_ptr<LifecycleControl> lifecycle_control;
    std::shared_ptr<EventSink> event_sink;
    bool disconnected;
    std::mutex observer_mutex;
};

}
}
}

#endif /* MIR_CLIENT_RPC_MIR_SOCKET_RPC_CHANNEL_H_ */
