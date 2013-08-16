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

#include <boost/asio.hpp>

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
namespace rpc
{

class RpcReport;

class MirSocketRpcChannel : public MirBasicRpcChannel
{
public:
    MirSocketRpcChannel(std::string const& endpoint,
                        std::shared_ptr<SurfaceMap> const& surface_map,
                        std::shared_ptr<DisplayConfiguration> const& disp_config,
                        std::shared_ptr<RpcReport> const& rpc_report,
                        std::shared_ptr<LifecycleControl> const& lifecycle_control);
    ~MirSocketRpcChannel();

private:
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method, google::protobuf::RpcController*,
        const google::protobuf::Message* parameters, google::protobuf::Message* response,
        google::protobuf::Closure* complete);
    std::shared_ptr<RpcReport> const rpc_report;
    detail::PendingCallCache pending_calls;
    std::thread io_service_thread;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work;
    boost::asio::local::stream_protocol::endpoint endpoint;
    boost::asio::local::stream_protocol::socket socket;

    static size_t const size_of_header = 2;
    unsigned char header_bytes[size_of_header];

    void receive_file_descriptors(google::protobuf::Message* response, google::protobuf::Closure* complete);
    void receive_file_descriptors(std::vector<int> &fds);
    void send_message(mir::protobuf::wire::Invocation const& body, detail::SendBuffer& buffer,
                      mir::protobuf::wire::Invocation const& invocation);
    void on_header_read(const boost::system::error_code& error);

    void read_message();
    void process_event_sequence(std::string const& event);

    size_t read_message_header();

    mir::protobuf::wire::Result read_message_body(const size_t body_size);

    std::shared_ptr<SurfaceMap> surface_map;
    std::shared_ptr<DisplayConfiguration> display_configuration;
    std::shared_ptr<LifecycleControl> lifecycle_control;
};

}
}
}

#endif /* MIR_CLIENT_RPC_MIR_SOCKET_RPC_CHANNEL_H_ */
