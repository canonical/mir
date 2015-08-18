/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/scene/null_session_listener.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

#include "mir_test_framework/interprocess_client_server_test.h"
#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/process.h"

#include "mir/test/cross_process_action.h"
#include "mir/test/fake_shared.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <future>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

using UnresponsiveClient = mtf::InterprocessClientServerTest;

namespace
{
struct SessionListener : ms::NullSessionListener
{
    void starting(std::shared_ptr<ms::Session> const& session) override
        { sessions.insert(session); }

    void stopping(std::shared_ptr<ms::Session> const& session) override
        { sessions.erase(session); }

    void for_each(std::function<void(std::shared_ptr<ms::Session> const&)> f) const
    {
        for (auto& session : sessions)
            f(session);
    }

private:
    std::set<std::shared_ptr<ms::Session>> sessions;
};
}

TEST_F(UnresponsiveClient, does_not_hang_server)
{
    mt::CrossProcessAction server_send_events;
    mt::CrossProcessAction client_connect;
    mt::CrossProcessAction client_release;
    mt::CrossProcessAction server_finish;

    SessionListener sessions;

    init_server([&]
         {
            server.override_the_session_listener([&]
                { return mt::fake_shared(sessions); });
         });

    run_in_server([&]
        {
            mt::AutoJoinThread t{
                [&] {
                    server_send_events.exec([&]
                        {
                            for (int i = 0; i < 1000; ++i)
                            {
                                sessions.for_each(
                                    [i] (std::shared_ptr<ms::Session> const& session)
                                    {
                                        session->default_surface()->resize({i + 1, i + 1});
                                    });
                            }
                        });
                }};

            server_finish.exec([]{});
        });

    auto const client_code = [&]
    {
        MirConnection* connection = nullptr;
        MirSurface* surface = nullptr;

        client_connect.exec([&]
            {
                connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

                surface = mtf::make_any_surface(connection);
            });

        client_release.exec([&]
            {
                // We would normally explicitly release the surface at this
                // point. However, because we have been filling the server
                // send socket buffer, releasing the surface may cause the
                // server to try to write to a full socket buffer when
                // responding, leading the server to believe that the client
                // is blocked, and causing a premature client disconnection.
                mir_connection_release(connection);
            });
    };

    auto client_process = new_client_process(client_code);

    if (is_test_process())
    {
        client_connect();
        kill(client_pid(), SIGSTOP);
        try 
        { 
            server_send_events(std::chrono::seconds{10});
        }
        catch(...)
        { 
            ADD_FAILURE() << "Server blocked while sending events";
        }
        kill(client_pid(), SIGCONT);
        client_release();
        server_finish();

        auto const result = client_process->wait_for_termination();
        EXPECT_THAT(result.exit_code, testing::Eq(EXIT_SUCCESS));
    }
}
