/*
 * Copyright © 2020 Canonical Ltd.
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

#include <miral/test_server.h>
#include <miral/external_client.h>
#include <miral/x11_support.h>

#include <mir/test/signal.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <miral/x11_support.h>

using namespace testing;

namespace
{
struct ExternalClient : miral::TestServer
{
    ExternalClient()
    {
        start_server_in_setup = false;
        add_server_init(external_client);
    }

    ~ExternalClient()
    {
        unlink(output.c_str());
    }

    miral::ExternalClientLauncher external_client;
    miral::X11Support x11;

    std::string const output = tmpnam(nullptr);

    auto client_env_value(std::string const& key) const -> std::string
    {
        auto const client_pid = external_client.launch({"bash", "-c", ("echo ${" + key + "} >" + output).c_str()});
        return get_client_env_value(client_pid);
    }

    auto client_env_x11_value(std::string const& key) const -> std::string
    {
        auto const client_pid = external_client.launch_using_x11({"bash", "-c", ("echo ${" + key + "} >" + output).c_str()});
        return get_client_env_value(client_pid);
    }

    std::string get_client_env_value(pid_t client_pid) const
    {
        int status;
        waitpid(client_pid, &status, 0);

        std::ifstream in{output};
        std::string result;
        getline(in, result);
        return result;
    }
    
    bool cannot_start_X_server()
    {
        // Starting an X server on LP builder, or Fedora CI, doesn't work
        return getenv("XDG_RUNTIME_DIR") == nullptr || access("/tmp/.X11-unix/", W_OK) == 0;
    }
};

auto const app_env = "MIR_SERVER_APP_ENV";
auto const app_x11_env = "MIR_SERVER_APP_ENV_X11";
}

TEST_F(ExternalClient, default_app_env_is_as_expected)
{
    if (getenv("XDG_RUNTIME_DIR") == nullptr)
        add_to_environment("XDG_RUNTIME_DIR", "/tmp");

    start_server();

    EXPECT_THAT(client_env_value("GDK_BACKEND"), StrEq("wayland"));
    EXPECT_THAT(client_env_value("QT_QPA_PLATFORM"), StrEq("wayland"));
    EXPECT_THAT(client_env_value("SDL_VIDEODRIVER"), StrEq("wayland"));
    EXPECT_THAT(client_env_value("NO_AT_BRIDGE"), StrEq("1"));
    EXPECT_THAT(client_env_value("_JAVA_AWT_WM_NONREPARENTING"), StrEq("1"));
}

TEST_F(ExternalClient, default_app_env_x11_is_as_expected)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_server_init(x11);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq("x11"));
    EXPECT_THAT(client_env_x11_value("QT_QPA_PLATFORM"), StrEq("xcb"));
    EXPECT_THAT(client_env_x11_value("SDL_VIDEODRIVER"), StrEq("x11"));
    EXPECT_THAT(client_env_x11_value("NO_AT_BRIDGE"), StrEq("1"));
    EXPECT_THAT(client_env_x11_value("_JAVA_AWT_WM_NONREPARENTING"), StrEq("1"));
}

TEST_F(ExternalClient, override_app_env_can_set_gdk_backend)
{
    if (getenv("XDG_RUNTIME_DIR") == nullptr)
        add_to_environment("XDG_RUNTIME_DIR", "/tmp");

    add_to_environment(app_env, "GDK_BACKEND=mir");
    start_server();

    EXPECT_THAT(client_env_value("GDK_BACKEND"), StrEq("mir"));
}

TEST_F(ExternalClient, override_app_env_x11_can_unset)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, "-GDK_BACKEND");
    add_server_init(x11);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq(""));
}

TEST_F(ExternalClient, override_app_env_x11_can_unset_and_set)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, "-GDK_BACKEND:QT_QPA_PLATFORM=xcb");
    add_server_init(x11);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq(""));
    EXPECT_THAT(client_env_x11_value("QT_QPA_PLATFORM"), StrEq("xcb"));
}

TEST_F(ExternalClient, override_app_env_x11_can_set_and_unset)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, "QT_QPA_PLATFORM=xcb:-GDK_BACKEND");
    add_server_init(x11);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq(""));
    EXPECT_THAT(client_env_x11_value("QT_QPA_PLATFORM"), StrEq("xcb"));
}

TEST_F(ExternalClient, stray_separators_are_ignored)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, "::QT_QPA_PLATFORM=xcb::-GDK_BACKEND::");
    add_server_init(x11);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq(""));
    EXPECT_THAT(client_env_x11_value("QT_QPA_PLATFORM"), StrEq("xcb"));
}

TEST_F(ExternalClient, empty_override_does_nothing)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, "");
    add_server_init(x11);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("QT_QPA_PLATFORM"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("SDL_VIDEODRIVER"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("NO_AT_BRIDGE"), StrEq("1"));
    EXPECT_THAT(client_env_x11_value("_JAVA_AWT_WM_NONREPARENTING"), StrEq("1"));
}

TEST_F(ExternalClient, strange_override_does_nothing)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, "=====");
    add_server_init(x11);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("QT_QPA_PLATFORM"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("SDL_VIDEODRIVER"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("NO_AT_BRIDGE"), StrEq("1"));
    EXPECT_THAT(client_env_x11_value("_JAVA_AWT_WM_NONREPARENTING"), StrEq("1"));
}

TEST_F(ExternalClient, another_strange_override_does_nothing)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, ":::");
    add_server_init(x11);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("QT_QPA_PLATFORM"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("SDL_VIDEODRIVER"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("NO_AT_BRIDGE"), StrEq("1"));
    EXPECT_THAT(client_env_x11_value("_JAVA_AWT_WM_NONREPARENTING"), StrEq("1"));
}
