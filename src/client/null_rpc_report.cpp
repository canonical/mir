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

#include "null_rpc_report.h"

namespace mcl = mir::client;

void mcl::NullRpcReport::invocation_requested(
    mir::protobuf::wire::Invocation const& /*invocation*/)
{
}

void mcl::NullRpcReport::invocation_succeeded(
    mir::protobuf::wire::Invocation const& /*invocation*/)
{
}

void mcl::NullRpcReport::invocation_failed(
    mir::protobuf::wire::Invocation const& /*invocation*/,
    boost::system::error_code const& /*error*/)
{
}

void mcl::NullRpcReport::header_receipt_failed(
    boost::system::error_code const& /*error*/)
{
}

void mcl::NullRpcReport::result_receipt_succeeded(
    mir::protobuf::wire::Result const& /*result*/)
{
}

void mcl::NullRpcReport::result_receipt_failed(
    std::exception const& /*ex*/)
{
}


void mcl::NullRpcReport::event_parsing_succeeded(
    MirEvent const& /*event*/)
{
}

void mcl::NullRpcReport::event_parsing_failed(
    mir::protobuf::Event const& /*event*/)
{
}

void mcl::NullRpcReport::orphaned_result(
    mir::protobuf::wire::Result const& /*result*/)
{
}

void mcl::NullRpcReport::complete_response(
    mir::protobuf::wire::Result const& /*result*/)
{

}

void mcl::NullRpcReport::result_processing_failed(
    mir::protobuf::wire::Result const& /*result*/,
    std::exception const& /*ex*/)
{
}

void mcl::NullRpcReport::file_descriptors_received(
    google::protobuf::Message const& /*response*/,
    std::vector<int32_t> const& /*fds*/)
{
}
