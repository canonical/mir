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


#include "mir_binder_rpc_channel.h"

#include "mir_protobuf.pb.h"  // For Buffer frig
#include "mir_protobuf_wire.pb.h"

#include <binder/IServiceManager.h>

namespace
{
// Too clever? The idea is to ensure protbuf version is verified once (on
// the first google_protobuf_guard() call) and memory is released on exit.
struct google_protobuf_guard_t
{
    google_protobuf_guard_t() { GOOGLE_PROTOBUF_VERIFY_VERSION; }
    ~google_protobuf_guard_t() { google::protobuf::ShutdownProtobufLibrary(); }
};

void google_protobuf_guard()
{
    static google_protobuf_guard_t guard;
}

bool force_init{(google_protobuf_guard(), true)};
}

namespace mcl = mir::client;

mcl::MirBinderRpcChannel::MirBinderRpcChannel()
{
    // TODO
}

mcl::MirBinderRpcChannel::MirBinderRpcChannel(
    std::string const& endpoint,
    std::shared_ptr<Logger> const& log) :
    binder(android::defaultServiceManager()->getService(android::String16(endpoint.c_str()))),
    log(log)
{
}

mcl::MirBinderRpcChannel::~MirBinderRpcChannel()
{
    // TODO
}

void mcl::MirBinderRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* /*method*/,
    google::protobuf::RpcController*,
    const google::protobuf::Message* /*parameters*/,
    google::protobuf::Message* /*response*/,
    google::protobuf::Closure* /*complete*/)
{
    // TODO
}

