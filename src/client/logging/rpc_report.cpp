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

#include "mir/logging/logger.h"

#include "mir_protobuf_wire.pb.h"

#include <boost/exception/diagnostic_information.hpp>
#include <sstream>

namespace ml = mir::logging;
namespace mcll = mir::client::logging;

namespace
{
std::string const component{"rpc"};
}

mcll::RpcReport::RpcReport(std::shared_ptr<ml::Logger> const& logger)
    : logger{logger}
{
}

void mcll::RpcReport::invocation_requested(
    mir::protobuf::wire::Invocation const& invocation)
{
    std::stringstream ss;
    ss << "Invocation request: id: " << invocation.id()
       << " method_name: " << invocation.method_name();

    logger->log(ml::Severity::debug, ss.str(), component);
}

void mcll::RpcReport::invocation_succeeded(
    mir::protobuf::wire::Invocation const& invocation)
{
    std::stringstream ss;
    ss << "Invocation succeeded: id: " << invocation.id()
       << " method_name: " << invocation.method_name();

    logger->log(ml::Severity::debug, ss.str(), component);
}

void mcll::RpcReport::invocation_failed(
    mir::protobuf::wire::Invocation const& invocation,
    std::exception const& ex)
{
    std::stringstream ss;
    ss << "Invocation failed: id: " << invocation.id()
       << " method_name: " << invocation.method_name()
       << " error: " << ex.what();

    logger->log(ml::Severity::error, ss.str(), component);
}

void mcll::RpcReport::result_receipt_succeeded(
    mir::protobuf::wire::Result const& result)
{
    std::stringstream ss;
    ss << "Result received: id: " << result.id();

    logger->log(ml::Severity::debug, ss.str(), component);
}

void mcll::RpcReport::result_receipt_failed(
    std::exception const& ex)
{
    std::stringstream ss;
    ss << "Result receipt failed: reason: " << ex.what();

    logger->log(ml::Severity::error, ss.str(), component);
}

void mcll::RpcReport::event_parsing_succeeded(
    MirEvent const& /*event*/)
{
    std::stringstream ss;
    /* TODO: Log more information about event */
    ss << "Event parsed";

    logger->log(ml::Severity::debug, ss.str(), component);
}

void mcll::RpcReport::event_parsing_failed(
    mir::protobuf::Event const& /*event*/)
{
    std::stringstream ss;
    /* TODO: Log more information about event */
    ss << "Event parsing failed";

    logger->log(ml::Severity::warning, ss.str(), component);
}

void mcll::RpcReport::orphaned_result(
    mir::protobuf::wire::Result const& result)
{
    std::stringstream ss;
    ss << "Orphaned result: " << result.id();

    logger->log(ml::Severity::error, ss.str(), component);
}

void mcll::RpcReport::complete_response(
    mir::protobuf::wire::Result const& result)
{
    std::stringstream ss;
    ss << "Complete response: id: " << result.id();

    logger->log(ml::Severity::debug, ss.str(), component);
}

void mcll::RpcReport::result_processing_failed(
    mir::protobuf::wire::Result const& /*result*/,
    std::exception const& ex)
{
    std::stringstream ss;
    ss << "Result processing failed: reason: " << ex.what();

    logger->log(ml::Severity::error, ss.str(), component);
}

void mcll::RpcReport::file_descriptors_received(
    google::protobuf::MessageLite const& /*response*/,
    std::vector<Fd> const& fds)
{
    std::stringstream ss;
    ss << "File descriptors received: ";
    for (auto f : fds)
        ss << f << " ";

    logger->log(ml::Severity::debug, ss.str(), component);
}
