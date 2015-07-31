/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_RPC_RPC_REPORT_H_
#define MIR_CLIENT_RPC_RPC_REPORT_H_

#include "mir_toolkit/event.h"
#include "mir/fd.h"

#include <exception>
#include <vector>

namespace google
{
namespace protobuf
{
class MessageLite;
}
}

namespace mir
{
namespace protobuf
{
class Event;

namespace wire
{
class Invocation;
class Result;
}
}
namespace client
{
namespace rpc
{

class RpcReport
{
public:
    virtual ~RpcReport() = default;

    virtual void invocation_requested(mir::protobuf::wire::Invocation const& invocation) = 0;
    virtual void invocation_succeeded(mir::protobuf::wire::Invocation const& invocation) = 0;
    virtual void invocation_failed(mir::protobuf::wire::Invocation const& invocation,
                                   std::exception const& ex) = 0;

    virtual void result_receipt_succeeded(mir::protobuf::wire::Result const& result) = 0;
    virtual void result_receipt_failed(std::exception const& ex) = 0;

    virtual void event_parsing_succeeded(MirEvent const& event) = 0;
    virtual void event_parsing_failed(mir::protobuf::Event const& event) = 0;

    virtual void orphaned_result(mir::protobuf::wire::Result const& result) = 0;
    virtual void complete_response(mir::protobuf::wire::Result const& result) = 0;

    virtual void result_processing_failed(mir::protobuf::wire::Result const& result,
                                          std::exception const& ex) = 0;

    virtual void file_descriptors_received(google::protobuf::MessageLite const& response,
                                           std::vector<mir::Fd> const& fds) = 0;

protected:
    RpcReport() = default;
    RpcReport(RpcReport const&) = delete;
    RpcReport& operator=(RpcReport const&) = delete;
};

}
}
}

#endif /* MIR_CLIENT_RPC_RPC_REPORT_H_ */
