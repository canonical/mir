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

#include "mir_test/test_protobuf_server.h"
#include "mir_test_doubles/stub_ipc_factory.h"
#include "mir_test_doubles/stub_session_authorizer.h"
#include "mir/frontend/connector_report.h"
#include "mir/frontend/protobuf_session_creator.h"
#include "src/server/frontend/published_socket_connector.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/report/null_report_factory.h"

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace mr = mir::report;

namespace
{
std::shared_ptr<mf::Connector> make_connector(
    std::string const& socket_name,
    std::shared_ptr<mf::ProtobufIpcFactory> const& factory,
    std::shared_ptr<mf::ConnectorReport> const& report)
{
    return std::make_shared<mf::PublishedSocketConnector>(
        socket_name,
        std::make_shared<mf::ProtobufSessionCreator>(
            factory,
            std::make_shared<mtd::StubSessionAuthorizer>(),
            mr::null_message_processor_report()),
        10,
        report);
}
}

mt::TestProtobufServer::TestProtobufServer(
    std::string const& socket_name,
    const std::shared_ptr<protobuf::DisplayServer>& tool) :
    TestProtobufServer(socket_name, tool, mr::null_connector_report())
{
}

mt::TestProtobufServer::TestProtobufServer(
    std::string const& socket_name,
    const std::shared_ptr<protobuf::DisplayServer>& tool,
    std::shared_ptr<frontend::ConnectorReport> const& report) :
    comm(make_connector(socket_name, std::make_shared<mtd::StubIpcFactory>(*tool), report))
{
}
