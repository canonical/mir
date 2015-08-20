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

#include "mir/test/test_protobuf_client.h"
#include "mir/test/doubles/mock_rpc_report.h"
#include "mir/test/doubles/null_client_event_sink.h"

#include "src/client/connection_surface_map.h"
#include "src/client/display_configuration.h"
#include "src/client/lifecycle_control.h"
#include "src/client/rpc/make_rpc_channel.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/events/event_private.h"

#include <thread>

namespace mtd = mir::test::doubles;
namespace mclr = mir::client::rpc;
namespace md = mir::dispatch;

mir::test::TestProtobufClient::TestProtobufClient(
    std::string socket_file,
    int timeout_ms) :
    rpc_report(std::make_shared<testing::NiceMock<doubles::MockRpcReport>>()),
    channel(mclr::make_rpc_channel(
        socket_file,
        std::make_shared<mir::client::ConnectionSurfaceMap>(),
        std::make_shared<mir::client::DisplayConfiguration>(),
        rpc_report,
        std::make_shared<mir::client::LifecycleControl>(),
        std::make_shared<mir::client::AtomicCallback<int32_t>>(),
        std::make_shared<mtd::NullClientEventSink>())),
    eventloop{std::make_shared<md::ThreadedDispatcher>("Mir/TestIPC", std::dynamic_pointer_cast<md::Dispatchable>(channel))},
    display_server(channel),
    maxwait(timeout_ms),
    connect_done_called(false),
    create_surface_called(false),
    next_buffer_called(false),
    exchange_buffer_called(false),
    release_surface_called(false),
    disconnect_done_called(false),
    drm_auth_magic_done_called(false),
    configure_display_done_called(false),
    tfd_done_called(false),
    connect_done_count(0),
    create_surface_done_count(0),
    disconnect_done_count(0)
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
    ON_CALL(*this, next_buffer_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_next_buffer_done));
    ON_CALL(*this, exchange_buffer_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_exchange_buffer_done));
    ON_CALL(*this, release_surface_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_release_surface_done));
    ON_CALL(*this, disconnect_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_disconnect_done));
    ON_CALL(*this, drm_auth_magic_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_drm_auth_magic_done));
    ON_CALL(*this, display_configure_done())
        .WillByDefault(testing::Invoke(this, &TestProtobufClient::on_configure_display_done));
    ON_CALL(*this, prompt_session_start_done())
        .WillByDefault(testing::Invoke(&wc_prompt_session_start, &WaitCondition::wake_up_everyone));
    ON_CALL(*this, prompt_session_stop_done())
        .WillByDefault(testing::Invoke(&wc_prompt_session_stop, &WaitCondition::wake_up_everyone));
}

void mir::test::TestProtobufClient::on_connect_done()
{
    connect_done_called.store(true);

    auto old = connect_done_count.load();

    while (!connect_done_count.compare_exchange_weak(old, old+1));
}

void mir::test::TestProtobufClient::on_create_surface_done()
{
    create_surface_called.store(true);

    auto old = create_surface_done_count.load();

    while (!create_surface_done_count.compare_exchange_weak(old, old+1));
}

void mir::test::TestProtobufClient::on_next_buffer_done()
{
    next_buffer_called.store(true);
}

void mir::test::TestProtobufClient::on_exchange_buffer_done()
{
    exchange_buffer_called.store(true);
}

void mir::test::TestProtobufClient::on_release_surface_done()
{
    release_surface_called.store(true);
}

void mir::test::TestProtobufClient::on_disconnect_done()
{
    disconnect_done_called.store(true);

    auto old = disconnect_done_count.load();

    while (!disconnect_done_count.compare_exchange_weak(old, old+1));
}

void mir::test::TestProtobufClient::on_drm_auth_magic_done()
{
    drm_auth_magic_done_called.store(true);
}

void mir::test::TestProtobufClient::on_configure_display_done()
{
    configure_display_done_called.store(true);
}

void mir::test::TestProtobufClient::wait_for_configure_display_done()
{
    for (int i = 0; !configure_display_done_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }

    configure_display_done_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_connect_done()
{
    for (int i = 0; !connect_done_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }

    connect_done_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_create_surface()
{
    for (int i = 0; !create_surface_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    create_surface_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_next_buffer()
{
    for (int i = 0; !next_buffer_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    next_buffer_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_exchange_buffer()
{
    for (int i = 0; !exchange_buffer_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    exchange_buffer_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_release_surface()
{
    for (int i = 0; !release_surface_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    release_surface_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_disconnect_done()
{
    for (int i = 0; !disconnect_done_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    disconnect_done_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_drm_auth_magic_done()
{
    for (int i = 0; !drm_auth_magic_done_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    drm_auth_magic_done_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_surface_count(int count)
{
    for (int i = 0; count != create_surface_done_count.load() && i < 10000; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::this_thread::yield();
    }
}

void mir::test::TestProtobufClient::wait_for_disconnect_count(int count)
{
    for (int i = 0; count != disconnect_done_count.load() && i < 10000; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::this_thread::yield();
    }
}

void mir::test::TestProtobufClient::tfd_done()
{
    tfd_done_called.store(true);
}

void mir::test::TestProtobufClient::wait_for_tfd_done()
{
    for (int i = 0; !tfd_done_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    tfd_done_called.store(false);
}

void mir::test::TestProtobufClient::wait_for_prompt_session_start_done()
{
    wc_prompt_session_start.wait_for_at_most_seconds(maxwait);
}

void mir::test::TestProtobufClient::wait_for_prompt_session_stop_done()
{
    wc_prompt_session_stop.wait_for_at_most_seconds(maxwait);
}
