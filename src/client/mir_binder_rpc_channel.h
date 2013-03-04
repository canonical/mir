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

#ifndef MIR_CLIENT_MIR_BINDER_RPC_CHANNEL_H_
#define MIR_CLIENT_MIR_BINDER_RPC_CHANNEL_H_

#include "mir_basic_rpc_channel.h"
#include "mir/thread/all.h"
#include "mir_logger.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include <binder/Binder.h>
#include <binder/IServiceManager.h>

#include <boost/asio.hpp>

#include <iosfwd>

namespace mir
{
namespace client
{
class MirBinderRpcChannel : public MirBasicRpcChannel
{
public:
    MirBinderRpcChannel(const std::string& endpoint, const std::shared_ptr<Logger>& log);
    ~MirBinderRpcChannel();

private:
    virtual void CallMethod(
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController*,
        const google::protobuf::Message* parameters,
        google::protobuf::Message* response,
        google::protobuf::Closure* complete);

    void remote_call(
        std::shared_ptr<android::Parcel> const& request,
        google::protobuf::Message* response,
        google::protobuf::Closure* complete);

    android::sp<android::IServiceManager> const sm;
    android::sp<android::IBinder> const mir_proxy;
    std::shared_ptr<Logger> const log;

    static const int threads = 3;
    std::thread io_service_thread[threads];
    boost::asio::io_service io_service;
    boost::asio::io_service::work work;
};
}
}

#endif /* MIR_CLIENT_MIR_BINDER_RPC_CHANNEL_H_ */
