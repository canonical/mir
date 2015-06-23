/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/geometry/rectangle.h"
#include "mir/scene/session.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/headless_test.h"
#include "mir/test/doubles/mock_window_manager.h"

#include "mir/test/fake_shared.h"
#include "mir/test/signal.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;
using namespace testing;
using namespace std::literals::chrono_literals;

namespace
{
MATCHER_P(WeakPtrEq, p, "")
{
    return !arg.owner_before(p) && !p.owner_before(arg);
}

std::vector<geom::Rectangle> const display_geometry
{
    {{  0, 0}, { 640,  480}},
    {{480, 0}, {1920, 1080}}
};


using NiceMockWindowManager = NiceMock<mtd::MockWindowManager>;

struct Client
{
    explicit Client(char const* connect_string) :
        connection{mir_connect_sync(connect_string, __PRETTY_FUNCTION__)}
    {
    }

    Client(Client&& source) :
        connection{nullptr}
    {
        std::swap(connection, source.connection);
    }

    auto surface_create() const -> MirSurface*
    {
        auto spec = mir_connection_create_spec_for_normal_surface(
            connection, 800, 600, mir_pixel_format_bgr_888);
        auto surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);

        return surface;
    }

    void disconnect()
    {
        if (connection)
            mir_connection_release(connection);

        connection = nullptr;
    }

    ~Client() noexcept
    {
        disconnect();
    }

    MirConnection* connection{nullptr};
};

struct CustomWindowManagement : mtf::HeadlessTest
{
    void SetUp() override
    {
        add_to_environment("MIR_SERVER_NO_FILE", "");

        initial_display_layout(display_geometry);

        server.override_the_window_manager_builder([this]
            (msh::FocusController*) { return mt::fake_shared(window_manager); });
    }

    void TearDown() override
    {
        stop_server();
    }

    Client connect_client()
    {
        return Client(new_connection().c_str());
    }

    NiceMockWindowManager window_manager;
};
}

TEST_F(CustomWindowManagement, display_layout_is_notified_on_startup)
{
    for(auto const& rect : display_geometry)
        EXPECT_CALL(window_manager, add_display(rect));

    start_server();
}

TEST_F(CustomWindowManagement, display_layout_is_notified_on_shutdown)
{
    start_server();

    for(auto const& rect : display_geometry)
        EXPECT_CALL(window_manager, remove_display(rect));
}

TEST_F(CustomWindowManagement, client_connect_adds_session)
{
    start_server();

    EXPECT_CALL(window_manager, add_session(_));
    connect_client();
}

TEST_F(CustomWindowManagement, client_disconnect_removes_session)
{
    start_server();
    auto client = connect_client();

    EXPECT_CALL(window_manager, remove_session(_));

    client.disconnect();
}

TEST_F(CustomWindowManagement, surface_create_adds_surface)
{
    start_server();

    auto const client = connect_client();

    EXPECT_CALL(window_manager, add_surface(_,_,_));

    auto const surface = client.surface_create();

    mir_surface_release_sync(surface);
}

TEST_F(CustomWindowManagement, surface_rename_modifies_surface)
{
    auto const new_title = __PRETTY_FUNCTION__;

    start_server();
    auto const client = connect_client();
    auto const surface = client.surface_create();

    EXPECT_CALL(window_manager, modify_surface(_,_,_));

    auto const spec = mir_connection_create_spec_for_changes(client.connection);
    mir_surface_spec_set_name(spec, new_title);
    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);

    mir_surface_release_sync(surface);
}

TEST_F(CustomWindowManagement, surface_release_removes_surface)
{
    start_server();

    auto const client = connect_client();
    auto const surface = client.surface_create();

    EXPECT_CALL(window_manager, remove_surface(_,_));

    mir_surface_release_sync(surface);
}

TEST_F(CustomWindowManagement, surface_is_associated_with_correct_client)
{
    start_server();

    const int no_of_clients = 17;
    std::weak_ptr<ms::Session> session[no_of_clients];
    std::vector<Client> client; client.reserve(no_of_clients);

    for (int i = 0; i != no_of_clients; ++i)
    {
        EXPECT_CALL(window_manager, add_session(_)).WillOnce(SaveArg<0>(&session[i]));

        client.emplace_back(connect_client());
    }

    for (int i = 0; i != no_of_clients; ++i)
    {
        // verify expectations for each client in turn
        Mock::VerifyAndClearExpectations(&window_manager);

        EXPECT_CALL(window_manager, add_surface(WeakPtrEq(session[i]),_,_));
        EXPECT_CALL(window_manager, remove_surface(WeakPtrEq(session[i]),_));

        auto const surface = client[i].surface_create();
        mir_surface_release_sync(surface);
    }
}

TEST_F(CustomWindowManagement, state_change_requests_are_associated_with_correct_surface)
{
    start_server();
    auto const client = connect_client();

    const int no_of_surfaces = 17;
    std::weak_ptr<ms::Surface> server_surface[no_of_surfaces];
    MirSurface* client_surface[no_of_surfaces] = {};

    for (int i = 0; i != no_of_surfaces; ++i)
    {
        auto const grab_server_surface = [i, &server_surface](
            std::shared_ptr<ms::Session> const& session,
            ms::SurfaceCreationParameters const& params,
            std::function<mf::SurfaceId(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)> const& build)
            {
                auto const result = build(session, params);
                server_surface[i] = session->surface(result);
                return result;
            };

        EXPECT_CALL(window_manager, add_surface(_,_,_))
            .WillOnce(Invoke(grab_server_surface));

        client_surface[i] = client.surface_create();
    }

    for (int i = 0; i != no_of_surfaces; ++i)
    {
        // verify expectations for each surface in turn
        Mock::VerifyAndClearExpectations(&window_manager);

        mt::Signal received;

        EXPECT_CALL(window_manager, set_surface_attribute(_, WeakPtrEq(server_surface[i]), mir_surface_attrib_state,_))
            .WillOnce(WithArg<3>(Invoke([&](int value) { received.raise(); return value; })));

        mir_surface_set_state(client_surface[i], mir_surface_state_maximized);

        received.wait_for(400ms);
    }

    for (auto const surface : client_surface)
        mir_surface_release_sync(surface);
}
