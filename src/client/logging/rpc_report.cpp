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

#include "../mir_logger.h"

#include "mir_protobuf_wire.pb.h"

namespace mcll = mir::client::logging;

mcll::RpcReport::RpcReport(std::shared_ptr<Logger> const& log)
    : log{log}
{
}

void mcll::RpcReport::invocation_requested(
    mir::protobuf::wire::Invocation const& invocation)
{
    log->debug() << "Invocation request: id: " << invocation.id()
                 << " method_name: " << invocation.method_name() << std::endl;
}

void mcll::RpcReport::invocation_succeeded(
    mir::protobuf::wire::Invocation const& invocation)
{
    log->debug() << "Invocation succeeded: id: " << invocation.id()
                 << " method_name: " << invocation.method_name() << std::endl;
}

void mcll::RpcReport::invocation_failed(
    mir::protobuf::wire::Invocation const& invocation,
    boost::system::error_code const& error)
{
    log->error() << "Invocation failed: id: " << invocation.id()
                 << " method_name: " << invocation.method_name()
                 << " error: " << error.message() << std::endl;
}

void mcll::RpcReport::header_receipt_failed(
    boost::system::error_code const& error)
{
    log->error() << "Header receipt failed: "
                 << " error: " << error.message() << std::endl;
}

void mcll::RpcReport::result_receipt_succeeded(
    mir::protobuf::wire::Result const& result)
{
    log->debug() << "Result received: id: " << result.id() << std::endl;
}

void mcll::RpcReport::result_receipt_failed(
    std::exception const& ex)
{
    log->error() << "Result receipt failed: reason: " << ex.what() << std::endl;
}


void mcll::RpcReport::event_parsing_succeeded(
    MirEvent const& /*event*/)
{
    /* TODO: Log more information about event */
    log->error() << "Event parsed" << std::endl;
}

void mcll::RpcReport::event_parsing_failed(
    mir::protobuf::Event const& /*event*/)
{
    /* TODO: Log more information about event */
    log->error() << "Event parsing failed" << std::endl;
}

void mcll::RpcReport::orphaned_result(
    mir::protobuf::wire::Result const& result)
{
    log->error() << "Orphaned result: " << result.ShortDebugString() << std::endl;
}

void mcll::RpcReport::complete_response(
    mir::protobuf::wire::Result const& result)
{
    log->debug() << "Complete response: id: " << result.id() << std::endl;
}

void mcll::RpcReport::result_processing_failed(
    mir::protobuf::wire::Result const& /*result*/,
    std::exception const& ex)
{
    log->error() << "Result processing failed: reason: " << ex.what() << std::endl;
}

void mcll::RpcReport::file_descriptors_received(
    google::protobuf::Message const& /*response*/,
    std::vector<int32_t> const& fds)
{
    log->debug() << "File descriptors received: ";
    for (auto f : fds)
        log->debug() << f << " ";
    log->debug() << std::endl;
}
