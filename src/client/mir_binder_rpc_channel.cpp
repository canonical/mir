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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#include "mir_binder_rpc_channel.h"

#include "mir/protobuf/google_protobuf_guard.h"

#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <utils/String8.h>

namespace mcl = mir::client;

mcl::MirBinderRpcChannel::MirBinderRpcChannel(
    std::string const& endpoint,
    std::shared_ptr<Logger> const& log) :
    sm(android::defaultServiceManager()),
    mir_proxy(sm->getService(android::String16(endpoint.c_str()))),
    log(log),
    work(io_service)
{
    if (!mir_proxy.get() || android::OK != mir_proxy->pingBinder())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Can't find MIR server"));
    }

    auto run_io_service = boost::bind(&boost::asio::io_service::run, &io_service);

    for (int i = 0; i != threads; ++i)
    {
        io_service_thread[i] = std::move(std::thread(run_io_service));
    }
}

mcl::MirBinderRpcChannel::~MirBinderRpcChannel()
{
    io_service.stop();

    for (int i = 0; i != threads; ++i)
    {
        if (io_service_thread[i].joinable())
        {
            io_service_thread[i].join();
        }
    }
}

void mcl::MirBinderRpcChannel::remote_call(
    std::shared_ptr<android::Parcel> const& request,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    android::Parcel wire_response;
    mir_proxy->transact(0, *request, &wire_response);
    const auto& receive_buffer = wire_response.readString8();
    std::string inefficient_copy(receive_buffer.string(), receive_buffer.string() + receive_buffer.size());
    std::istringstream in(inefficient_copy);
    mir::protobuf::wire::Result result;
    result.ParseFromIstream(&in);
    response->ParseFromString(result.response());
    // Receive file descriptors
    {
        auto buffer = dynamic_cast<mir::protobuf::Buffer*>(response);
        if (!buffer)
        {
            auto surface = dynamic_cast<mir::protobuf::Surface*>(response);
            if (surface && surface->has_buffer())
                buffer = surface->mutable_buffer();
        }
        if (buffer)
        {
            buffer->clear_fd();
            const auto end = buffer->fds_on_side_channel();
            log->debug() << __PRETTY_FUNCTION__ << " expect " << end << " file descriptors" << std::endl;
            for (auto i = 0; i != end; ++i)
            {
                const auto fd = wire_response.readFileDescriptor();
                buffer->add_fd(fd);
            }
        }
        auto platform = dynamic_cast<mir::protobuf::Platform*>(response);
        if (!platform)
        {
            auto connection = dynamic_cast<mir::protobuf::Connection*>(response);
            if (connection && connection->has_platform())
                platform = connection->mutable_platform();
        }
        if (platform)
        {
            platform->clear_fd();
            const auto end = platform->fds_on_side_channel();
            log->debug() << __PRETTY_FUNCTION__ << " expect " << end << " file descriptors" << std::endl;
            for (auto i = 0; i != end; ++i)
            {
                const auto fd = wire_response.readFileDescriptor();
                platform->add_fd(fd);
            }
        }
    }
    complete->Run();
}

void mcl::MirBinderRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController*,
    const google::protobuf::Message* parameters,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    mir::protobuf::wire::Invocation invocation = invocation_for(method, parameters);
    std::ostringstream send_buffer;
    invocation.SerializeToOstream(&send_buffer);

    auto const& message = send_buffer.str();
    auto request = std::make_shared<android::Parcel>();

    request->writeString8(android::String8(message.data(), message.length()));

    io_service.dispatch([=] {remote_call(request, response, complete);});
}
