/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_toolkit/mir_client_library_lightdm.h"

#include "mir/frontend/surface_id.h"
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/frontend/shell.h"

#include "mir_test_doubles/mock_display.h"
#include "mir_test_framework/display_server_test_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <thread>
#include <fcntl.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace mir
{
namespace
{
struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon() :
        connection(0),
        surface(0)
    {
    }

    static void connection_callback(MirConnection* connection, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connection = connection;
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_created(surface);
    }

    static void next_buffer_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->next_buffer(surface);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_released(surface);
    }

    virtual void connected(MirConnection* new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirSurface* new_surface)
    {
        surface = new_surface;
    }

    virtual void next_buffer(MirSurface*)
    {
    }

    virtual void surface_released(MirSurface* /*released_surface*/)
    {
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;

    void set_flag(const char* const flag_file)
    {
        close(open(flag_file, O_CREAT, S_IWUSR | S_IRUSR));
    }

    void wait_for(const char* const flag_file)
    {
        int fd = -1;
        while (-1 == (fd = open(flag_file, O_RDONLY, S_IWUSR | S_IRUSR)))
        {
            // Wait for focus_config process to init
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        close(fd);
    }
};

class MockShell : public frontend::Shell
{
public:
    MockShell(std::shared_ptr<Shell> const& impl) :
        impl(impl)
    {
        using namespace ::testing;
        using frontend::Shell;
        ON_CALL(*this, open_session(_, _)).WillByDefault(Invoke(impl.get(), &Shell::open_session));
        ON_CALL(*this, close_session(_)).WillByDefault(Invoke(impl.get(), &Shell::close_session));

        ON_CALL(*this, tag_session_with_lightdm_id(_, _)).WillByDefault(Invoke(impl.get(), &Shell::tag_session_with_lightdm_id));
        ON_CALL(*this, focus_session_with_lightdm_id(_)).WillByDefault(Invoke(impl.get(), &Shell::focus_session_with_lightdm_id));

        ON_CALL(*this, create_surface_for(_, _)).WillByDefault(Invoke(impl.get(), &Shell::create_surface_for));
    }

    MOCK_METHOD2(open_session, std::shared_ptr<mf::Session> (std::string const& name, std::shared_ptr<mir::events::EventSink> const&));
    MOCK_METHOD1(close_session, void (std::shared_ptr<mf::Session> const& session));

    MOCK_METHOD2(tag_session_with_lightdm_id, void (std::shared_ptr<mf::Session> const& session, int id));
    MOCK_METHOD1(focus_session_with_lightdm_id, void (int id));

    MOCK_METHOD2(create_surface_for, frontend::SurfaceId(std::shared_ptr<mf::Session> const&, mf::SurfaceCreationParameters const&));

private:
    std::shared_ptr<frontend::Shell> const impl;
};
}

TEST_F(BespokeDisplayServerTestFixture, focus_management)
{
    struct ServerConfig : TestingServerConfiguration
    {
        mir::CachedPtr<mf::Shell> frontend_shell;

        std::shared_ptr<mf::Shell>
        the_frontend_shell() override
        {
            return frontend_shell(
                [this]() -> std::shared_ptr<mf::Shell>
                {
                    using namespace ::testing;

                    auto const& mock_shell = std::make_shared<MockShell>(
                        DefaultServerConfiguration::the_frontend_shell());

                    Sequence s1, s2;

                    EXPECT_CALL(*mock_shell, open_session(_, _))
                        .InSequence(s1, s2);

                    EXPECT_CALL(*mock_shell, create_surface_for(_,_))
                        .InSequence(s1);

                    EXPECT_CALL(*mock_shell, open_session(_, _))
                        .InSequence(s2);

                    EXPECT_CALL(*mock_shell, close_session(_))
                        .Times(2)
                        .InSequence(s1, s2);

                    {
                        using namespace testing;
                        InSequence test;

                        EXPECT_CALL(*mock_shell, tag_session_with_lightdm_id(_, _));
                        EXPECT_CALL(*mock_shell, focus_session_with_lightdm_id(_));
                    }

                    return mock_shell;
                });
        }
    } server_config;

    launch_server_process(server_config);

    int const lightdm_id = __LINE__;
    static char const* const focus_ready = "focus_management_focus_client_ready.tmp";
    static char const* const manager_done = "focus_management_focus_manager_done.tmp";
    std::remove(focus_ready);
    std::remove(manager_done);

    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect_with_lightdm_id(
                mir_test_socket,
                lightdm_id,
                __PRETTY_FUNCTION__,
                connection_callback,
                this));

            ASSERT_TRUE(connection != NULL);

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));

            set_flag(focus_ready);

            wait_for(manager_done);
            mir_connection_release(connection);
        }
    } focus_config;

    struct LightdmConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect(
                mir_test_socket,
                __PRETTY_FUNCTION__,
                connection_callback,
                this));

            wait_for(focus_ready);
            mir_select_focus_by_lightdm_id(connection, lightdm_id);

            mir_connection_release(connection);
            set_flag(manager_done);
        }
    } lightdm_config;

    launch_client_process(focus_config);
    launch_client_process(lightdm_config);
}
}
