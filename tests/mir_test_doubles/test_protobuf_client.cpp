/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/test/test_protobuf_client.h"
#include "mir/test/doubles/mock_rpc_report.h"
#include "mir/input/null_input_receiver_report.h"
#include "mir/test/doubles/null_client_event_sink.h"

#include "src/client/connection_surface_map.h"
#include "src/client/display_configuration.h"
#include "src/client/lifecycle_control.h"
#include "src/client/buffer_factory.h"
#include "src/client/rpc/make_rpc_channel.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "mir/input/input_devices.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/events/event_private.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <thread>

namespace mtd = mir::test::doubles;
namespace mclr = mir::client::rpc;
namespace md = mir::dispatch;

mir::test::TestProtobufClient::TestProtobufClient(std::string socket_file, int timeout_ms) :
    rpc_report(std::make_shared<testing::NiceMock<doubles::MockRpcReport>>()),
    surface_map{std::make_shared<mir::client::ConnectionSurfaceMap>()},
    channel(mclr::make_rpc_channel(
        socket_file,
        surface_map,
        std::make_shared<mir::client::BufferFactory>(),
        std::make_shared<mir::client::DisplayConfiguration>(),
        std::make_shared<mir::input::InputDevices>(surface_map),
        rpc_report,
        input_report,
        std::make_shared<mir::client::LifecycleControl>(),
        std::make_shared<mir::client::AtomicCallback<int32_t>>(),
        std::make_shared<mir::client::AtomicCallback<MirError const*>>(),
        std::make_shared<mtd::NullClientEventSink>())),
    eventloop{std::make_shared<md::ThreadedDispatcher>("Mir/TestIPC", std::dynamic_pointer_cast<md::Dispatchable>(channel))},
    display_server(channel),
    maxwait(timeout_ms),
    connect_done_called(false),
    create_surface_called(false),
    submit_buffer_called(false),
    disconnect_done_called(false),
    configure_display_done_called(false),
    create_surface_done_count(0)
{
    surface_parameters.set_width(640);
    surface_parameters.set_height(480);
    surface_parameters.set_pixel_format(0);
    surface_parameters.set_buffer_usage(0);
    surface_parameters.set_output_id(mir_display_output_id_invalid);

    prompt_session_parameters.set_application_pid(__LINE__);

    ON_CALL(*this, connect_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_connect_done));
    ON_CALL(*this, create_surface_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_create_surface_done));
    ON_CALL(*this, submit_buffer_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_submit_buffer_done));
    ON_CALL(*this, disconnect_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_disconnect_done));
    ON_CALL(*this, display_configure_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_configure_display_done));
}

void mir::test::TestProtobufClient::signal_condition(bool& condition)
{
    std::lock_guard<decltype(guard)> lk{guard};
    condition = true;
    cv.notify_all();
}

void mir::test::TestProtobufClient::reset_condition(bool& condition)
{
    std::lock_guard<decltype(guard)> lk{guard};
    condition = false;
}

void mir::test::TestProtobufClient::on_connect_done()
{
    signal_condition(connect_done_called);
}

void mir::test::TestProtobufClient::on_create_surface_done()
{
    std::lock_guard<decltype(guard)> lk{guard};
    create_surface_called = true;
    create_surface_done_count++;
    cv.notify_all();
}

void mir::test::TestProtobufClient::on_submit_buffer_done()
{
    signal_condition(submit_buffer_called);
}

void mir::test::TestProtobufClient::on_disconnect_done()
{
    signal_condition(disconnect_done_called);
}

void mir::test::TestProtobufClient::on_configure_display_done()
{
    signal_condition(configure_display_done_called);
}

void mir::test::TestProtobufClient::connect()
{
    reset_condition(connect_done_called);
    display_server.connect(
        &connect_parameters,
        &connection,
        google::protobuf::NewCallback(this, &TestProtobufClient::connect_done));
}

void mir::test::TestProtobufClient::disconnect()
{
    reset_condition(disconnect_done_called);
    display_server.disconnect(
        &ignored,
        &ignored,
        google::protobuf::NewCallback(this, &TestProtobufClient::disconnect_done));
}

void mir::test::TestProtobufClient::create_surface()
{
    reset_condition(create_surface_called);
    display_server.create_surface(
        &surface_parameters,
        &surface,
        google::protobuf::NewCallback(this, &TestProtobufClient::create_surface_done));
}

void mir::test::TestProtobufClient::submit_buffer()
{
    reset_condition(submit_buffer_called);
    display_server.submit_buffer(
        &buffer_request,
        &ignored,
        google::protobuf::NewCallback(this, &TestProtobufClient::submit_buffer_done));
}

void mir::test::TestProtobufClient::configure_display()
{
    reset_condition(configure_display_done_called);
    display_server.configure_display(
        &disp_config,
        &disp_config_response,
        google::protobuf::NewCallback(this, &TestProtobufClient::display_configure_done));
}

void mir::test::TestProtobufClient::wait_for_configure_display_done()
{
    wait_for([this]{ return configure_display_done_called; }, "Timed out waiting to configure display");
}

void mir::test::TestProtobufClient::wait_for_connect_done()
{
    wait_for([this]{ return connect_done_called; }, "Timed out waiting to connect");
}

void mir::test::TestProtobufClient::wait_for_create_surface()
{
    wait_for([this]{ return create_surface_called; }, "Timed out waiting create surface");
}

void mir::test::TestProtobufClient::wait_for_submit_buffer()
{
    wait_for([this] { return submit_buffer_called; }, "Timed out waiting for next buffer");
}

void mir::test::TestProtobufClient::wait_for_disconnect_done()
{
    wait_for([this] { return disconnect_done_called; }, "Timed out waiting to disconnect");
}

void mir::test::TestProtobufClient::wait_for_surface_count(int count)
{
    wait_for([this, count] { return create_surface_done_count == count; }, "Timed out waiting for surface count");
}

void mir::test::TestProtobufClient::wait_for(std::function<bool()> const& predicate, std::string const& error_message)
{
    std::unique_lock<decltype(guard)> lk{guard};
    if (!cv.wait_for(lk, std::chrono::milliseconds(maxwait), predicate))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(error_message));
    }
}
