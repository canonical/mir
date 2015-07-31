/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_RPC_REPORT_H_
#define MIR_TEST_DOUBLES_MOCK_RPC_REPORT_H_

#include "src/client/rpc/rpc_report.h"

#include "mir_protobuf_wire.pb.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockRpcReport : public mir::client::rpc::RpcReport
{
public:
    ~MockRpcReport() noexcept {}

    MOCK_METHOD1(invocation_requested,
                 void(mir::protobuf::wire::Invocation const&));
    MOCK_METHOD1(invocation_succeeded,
                 void(mir::protobuf::wire::Invocation const&));
    MOCK_METHOD2(invocation_failed,
                 void(mir::protobuf::wire::Invocation const&,
                      std::exception const&));

    MOCK_METHOD1(result_receipt_succeeded,
                 void(mir::protobuf::wire::Result const&));
    MOCK_METHOD1(result_receipt_failed,
                 void(std::exception const&));

    MOCK_METHOD1(event_parsing_succeeded,
                 void(MirEvent const&));
    MOCK_METHOD1(event_parsing_failed,
                 void(mir::protobuf::Event const&));

    MOCK_METHOD1(orphaned_result,
                 void(mir::protobuf::wire::Result const&));
    MOCK_METHOD1(complete_response,
                 void(mir::protobuf::wire::Result const&));

    MOCK_METHOD2(result_processing_failed,
                 void(mir::protobuf::wire::Result const&,
                      std::exception const& ex));

    MOCK_METHOD2(file_descriptors_received,
                 void(google::protobuf::MessageLite const&,
                      std::vector<Fd> const&));
};


}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_RPC_REPORT_H_ */
