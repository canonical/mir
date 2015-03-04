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

#include "mir/shell/generic_shell.h"
#include "mir/geometry/rectangle.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/headless_test.h"
#include "mir_test_doubles/mock_window_manager.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;
using namespace testing;

namespace
{
std::vector<geom::Rectangle> const display_geometry
{
    {{  0, 0}, { 640,  480}},
    {{480, 0}, {1920, 1080}}
};


using NiceMockWindowManager = NiceMock<mtd::MockWindowManager>;

struct Client
{
    explicit Client(mtf::AsyncServerRunner& server_runner) :
        connection{mir_connect_sync(server_runner.new_connection().c_str(), __PRETTY_FUNCTION__)}
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

        server.override_the_shell([this]
           {
                return std::make_shared<msh::GenericShell>(
                    server.the_input_targeter(),
                    server.the_surface_coordinator(),
                    server.the_session_coordinator(),
                    server.the_prompt_session_manager(),
                    [this](msh::FocusController*) { return mt::fake_shared(window_manager); });
           });
    }

    void TearDown() override
    {
        stop_server();
    }

    Client connect_client()
    {
        return Client(*this);
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

TEST_F(CustomWindowManagement, surface_release_removes_surface)
{
    start_server();

    auto const client = connect_client();
    auto const surface = client.surface_create();

    EXPECT_CALL(window_manager, remove_surface(_,_));

    EXPECT_TRUE(mir_surface_is_valid(surface));
    mir_surface_release_sync(surface);
    Mock::VerifyAndClearExpectations(&window_manager);
}

TEST_F(CustomWindowManagement, surface_is_associated_with_correct_client)
{
    start_server();

    std::shared_ptr<ms::Session> session1;
    std::shared_ptr<ms::Session> session2;

    EXPECT_CALL(window_manager, add_session(_))
        .WillOnce(SaveArg<0>(&session1))
        .WillOnce(SaveArg<0>(&session2));

    auto const client1 = connect_client();
    auto const client2 = connect_client();

    std::shared_ptr<ms::Session> session_for_add;
    std::shared_ptr<ms::Session> session_for_remove;

    EXPECT_CALL(window_manager, add_surface(_,_,_)).WillOnce(DoAll(
        SaveArg<0>(&session_for_add),
        Invoke(NiceMockWindowManager::add_surface_default)));

    EXPECT_CALL(window_manager, remove_surface(_,_))
        .WillOnce(SaveArg<0>(&session_for_remove));

    auto const surface = client1.surface_create();
    mir_surface_release_sync(surface);

    EXPECT_THAT(session1, Ne(session2));
    EXPECT_THAT(session_for_add, Eq(session1));
    EXPECT_THAT(session_for_remove, Eq(session1));
}
