/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_CLIENT_MIR_BINDER_RPC_CHANNEL_H_
#define MIR_CLIENT_MIR_BINDER_RPC_CHANNEL_H_

#include "mir_basic_rpc_channel.h"
#include "mir/thread/all.h"
#include "mir_client/mir_logger.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include <binder/Binder.h>
#include <binder/IServiceManager.h>

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

    android::sp<android::IServiceManager> const sm;
    android::sp<android::IBinder> const mir_proxy;
    std::shared_ptr<Logger> const log;
};
}
}

#endif /* MIR_CLIENT_MIR_BINDER_RPC_CHANNEL_H_ */
