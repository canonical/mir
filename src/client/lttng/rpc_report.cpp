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

#include "rpc_report.h"
#include "mir/report/lttng/mir_tracepoint.h"

#include "mir_protobuf_wire.pb.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "rpc_report_tp.h"

namespace mcl = mir::client;

void mcl::lttng::RpcReport::invocation_requested(
    mir::protobuf::wire::Invocation const& invocation)
{
    mir_tracepoint(mir_client_rpc, invocation_requested,
                   invocation.id(), invocation.method_name().c_str());
}

void mcl::lttng::RpcReport::invocation_succeeded(
    mir::protobuf::wire::Invocation const& invocation)
{
    mir_tracepoint(mir_client_rpc, invocation_succeeded,
                   invocation.id(), invocation.method_name().c_str());
}

void mcl::lttng::RpcReport::invocation_failed(
    mir::protobuf::wire::Invocation const& /*invocation*/,
    std::exception const& /*ex*/)
{
}

void mcl::lttng::RpcReport::result_receipt_succeeded(
    mir::protobuf::wire::Result const& result)
{
    mir_tracepoint(mir_client_rpc, result_receipt_succeeded, result.id());
}

void mcl::lttng::RpcReport::result_receipt_failed(
    std::exception const& /*ex*/)
{
}

void mcl::lttng::RpcReport::event_parsing_succeeded(
    MirEvent const& /*event*/)
{
    /* TODO: Record more information about event */
    mir_tracepoint(mir_client_rpc, event_parsing_succeeded, 0);
}

void mcl::lttng::RpcReport::event_parsing_failed(
    mir::protobuf::Event const& /*event*/)
{
}

void mcl::lttng::RpcReport::orphaned_result(
    mir::protobuf::wire::Result const& /*result*/)
{
}

void mcl::lttng::RpcReport::complete_response(
    mir::protobuf::wire::Result const& result)
{
    mir_tracepoint(mir_client_rpc, complete_response, result.id());
}

void mcl::lttng::RpcReport::result_processing_failed(
    mir::protobuf::wire::Result const& /*result*/,
    std::exception const& /*ex*/)
{
}

void mcl::lttng::RpcReport::file_descriptors_received(
    google::protobuf::MessageLite const& /*response*/,
    std::vector<Fd> const& fds)
{
    auto handles = std::make_unique<int[]>(fds.size());
    for (unsigned i = 0 ; i < fds.size() ; ++i)
    {
        handles[i] = fds[i];
    }
    mir_tracepoint(mir_client_rpc, file_descriptors_received,
                   handles.get(), fds.size());
}
