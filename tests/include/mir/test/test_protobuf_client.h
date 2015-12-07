/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_TEST_CLIENT_H_
#define MIR_TEST_TEST_CLIENT_H_

#include "mir_protobuf.pb.h"
#include "mir/test/wait_condition.h"
#include "src/client/rpc/mir_display_server.h"

#include <gmock/gmock.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

namespace mir
{
namespace client { namespace rpc {class MirBasicRpcChannel;}}
namespace dispatch
{
class ThreadedDispatcher;
}
namespace test
{
namespace doubles
{
class MockRpcReport;
}
struct TestProtobufClient
{
    TestProtobufClient(std::string socket_file, int timeout_ms);

    std::shared_ptr<doubles::MockRpcReport> rpc_report;
    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> channel;
    std::shared_ptr<dispatch::ThreadedDispatcher> eventloop;
    mir::client::rpc::DisplayServer display_server;
    mir::protobuf::ConnectParameters connect_parameters;
    mir::protobuf::SurfaceParameters surface_parameters;
    mir::protobuf::Surface surface;
    mir::protobuf::Void ignored;
    mir::protobuf::Connection connection;
    mir::protobuf::DisplayConfiguration disp_config;
    mir::protobuf::DisplayConfiguration disp_config_response;
    mir::protobuf::PromptSessionParameters prompt_session_parameters;
    mir::protobuf::Void prompt_session;

    MOCK_METHOD0(connect_done, void());
    MOCK_METHOD0(create_surface_done, void());
    MOCK_METHOD0(next_buffer_done, void());
    MOCK_METHOD0(disconnect_done, void());
    MOCK_METHOD0(display_configure_done, void());

    void on_connect_done();
    void on_create_surface_done();
    void on_next_buffer_done();
    void on_disconnect_done();
    void on_configure_display_done();

    void wait_for_connect_done();
    void wait_for_create_surface();
    void wait_for_next_buffer();
    void wait_for_disconnect_done();
    void wait_for_configure_display_done();
    void wait_for_surface_count(int count);

    void wait_for(std::function<bool()> const& predicate, std::string const& error_message);
    void signal_condition(bool& condition);

    int const maxwait;
    bool connect_done_called;
    bool create_surface_called;
    bool next_buffer_called;
    bool exchange_buffer_called;
    bool disconnect_done_called;
    bool configure_display_done_called;
    int create_surface_done_count;

    std::mutex mutable guard;
    std::condition_variable cv;
};
}
}
#endif /* MIR_TEST_TEST_CLIENT_H_ */
