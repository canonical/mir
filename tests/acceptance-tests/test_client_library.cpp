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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_toolkit/mir_client_library.h"
#include "src/client/client_buffer.h"

#include "mir/frontend/communicator.h"

#include "src/client/mir_logger.h"

#include "mir_protobuf.pb.h"

#include <gtest/gtest.h>
#include <chrono>

namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace mcl = mir::client;
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
    ClientConfigCommon()
        : connection(0)
        , surface(0),
        buffers(0)
    {
    }

    static void connection_callback(MirConnection * connection, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connection = connection;
    }

    static void create_surface_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_created(surface);
    }

    static void next_buffer_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->next_buffer(surface);
    }

    static void release_surface_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_released(surface);
    }

    virtual void connected(MirConnection * new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirSurface * new_surface)
    {
        surface = new_surface;
    }

    virtual void next_buffer(MirSurface*)
    {
        ++buffers;
    }

    virtual void surface_released(MirSurface * /*released_surface*/)
    {
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;
    int buffers;
};
}

TEST_F(DefaultDisplayServerTestFixture, client_library_connects_and_disconnects)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_creates_surface)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {

            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));

            ASSERT_TRUE(surface != NULL);
            EXPECT_TRUE(mir_surface_is_valid(surface));
            EXPECT_STREQ(mir_surface_get_error_message(surface), "");

            MirSurfaceParameters response_params;
            mir_surface_get_parameters(surface, &response_params);
            EXPECT_EQ(request_params.width, response_params.width);
            EXPECT_EQ(request_params.height, response_params.height);
            EXPECT_EQ(request_params.pixel_format, response_params.pixel_format);
            EXPECT_EQ(request_params.buffer_usage, response_params.buffer_usage);


            mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

            ASSERT_TRUE(surface == NULL);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, surface_types)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {

            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));

            ASSERT_TRUE(surface != NULL);
            EXPECT_TRUE(mir_surface_is_valid(surface));
            EXPECT_STREQ(mir_surface_get_error_message(surface), "");

            EXPECT_EQ(mir_surface_get_type(surface),
                      mir_surface_type_normal);

            mir_wait_for(mir_surface_set_type(surface,
                                              mir_surface_type_freestyle));
            EXPECT_EQ(mir_surface_get_type(surface),
                      mir_surface_type_freestyle);

            mir_wait_for(mir_surface_set_type(surface,
                                            static_cast<MirSurfaceType>(999)));
            EXPECT_EQ(mir_surface_get_type(surface),
                      mir_surface_type_freestyle);

            mir_wait_for(mir_surface_set_type(surface,
                                              mir_surface_type_dialog));
            EXPECT_EQ(mir_surface_get_type(surface),
                      mir_surface_type_dialog);

            mir_wait_for(mir_surface_set_type(surface,
                                            static_cast<MirSurfaceType>(888)));
            EXPECT_EQ(mir_surface_get_type(surface),
                      mir_surface_type_dialog);

            // Stress-test synchronization logic with some flooding
            for (int i = 0; i < 1000; i++)
            {
                mir_surface_set_type(surface, mir_surface_type_normal);
                mir_surface_set_type(surface, mir_surface_type_utility);
                mir_surface_set_type(surface, mir_surface_type_dialog);
                mir_surface_set_type(surface, mir_surface_type_overlay);
                mir_surface_set_type(surface, mir_surface_type_freestyle);
                mir_wait_for(mir_surface_set_type(surface,
                                                  mir_surface_type_popover));
                ASSERT_EQ(mir_surface_get_type(surface),
                          mir_surface_type_popover);
            }

            mir_wait_for(mir_surface_release(surface, release_surface_callback,
                                             this));

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_creates_multiple_surfaces)
{
    int const n_surfaces = 13;

    struct ClientConfig : ClientConfigCommon
    {
        ClientConfig(int n_surfaces) : n_surfaces(n_surfaces)
        {
        }

        void surface_created(MirSurface * new_surface)
        {
            surfaces.insert(new_surface);
        }

        void surface_released(MirSurface * surface)
        {
            surfaces.erase(surface);
        }

        MirSurface * any_surface()
        {
            return *surfaces.begin();
        }

        size_t current_surface_count()
        {
            return surfaces.size();
        }

        void exec()
        {
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            for (int i = 0; i != n_surfaces; ++i)
            {
                old_surface_count = current_surface_count();

                MirSurfaceParameters const request_params =
                {
                    __PRETTY_FUNCTION__,
                    640, 480,
                    mir_pixel_format_abgr_8888,
                    mir_buffer_usage_hardware
                };

                mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));

                ASSERT_EQ(old_surface_count + 1, current_surface_count());
            }
            for (int i = 0; i != n_surfaces; ++i)
            {
                old_surface_count = current_surface_count();

                ASSERT_NE(old_surface_count, 0u);

                MirSurface * surface = any_surface();
                mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

                ASSERT_EQ(old_surface_count - 1, current_surface_count());
            }

            mir_connection_release(connection);
        }

        int n_surfaces;
        std::set<MirSurface *> surfaces;
        size_t old_surface_count;
    } client_config(n_surfaces);

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_accesses_and_advances_buffers)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {

            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ("", mir_connection_get_error_message(connection));

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));
            ASSERT_TRUE(surface != NULL);

            mir_wait_for(mir_surface_next_buffer(surface, next_buffer_callback, this));

            mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

            ASSERT_TRUE(surface == NULL);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_accesses_platform_package)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));
            ASSERT_TRUE(connection != NULL);

            MirPlatformPackage platform_package;
            platform_package.data_items = -1;
            platform_package.fd_items = -1;

            mir_connection_get_platform(connection, &platform_package);
            EXPECT_GE(0, platform_package.data_items);
            EXPECT_GE(0, platform_package.fd_items);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_accesses_display_info)
{
    static const int default_display_width = 1600, default_display_height = 1600;

    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));
            ASSERT_TRUE(connection != NULL);

            MirDisplayInfo display_info;
            display_info.width = -1;
            display_info.height = -1;

            mir_connection_get_display_info(connection, &display_info);
            EXPECT_GE(default_display_width, display_info.width);
            EXPECT_GE(default_display_height, display_info.height);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, connect_errors_handled)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect("garbage", __PRETTY_FUNCTION__, connection_callback, this));
            ASSERT_TRUE(connection != NULL);

            char const* error = mir_connection_get_error_message(connection);

            if (std::strcmp("connect: No such file or directory", error) &&
                std::strcmp("Can't find MIR server", error))
            {
                FAIL() << error;
            }
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, connect_errors_dont_blow_up)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect("garbage", __PRETTY_FUNCTION__, connection_callback, this));

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));
// TODO surface_create needs to fail safe too. After that is done we should add the following:
// TODO    mir_wait_for(mir_surface_next_buffer(surface, next_buffer_callback, this));
// TODO    mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
}
