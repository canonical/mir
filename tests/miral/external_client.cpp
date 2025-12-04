/*
 * Copyright © Canonical Ltd.
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
 */

#include <miral/test_server.h>
#include <miral/external_client.h>
#include <miral/x11_support.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>

using namespace testing;

namespace
{
struct ExternalClient : miral::TestServer
{
    ExternalClient()
    {
        char tmp_dir_path[] = "/tmp/mir-XXXXXX";
        if (mkdtemp(tmp_dir_path) == nullptr)
            throw std::runtime_error("failed to create temporary directory");
        output_dir = std::string(tmp_dir_path);
        output = output_dir + "/output";

        start_server_in_setup = false;
        add_server_init(external_client);
    }

    ~ExternalClient()
    {
        std::filesystem::remove_all(output_dir);
    }

    miral::ExternalClientLauncher external_client;
    miral::X11Support x11_disabled_by_default{};
    miral::X11Support x11_enabled_by_default{miral::X11Support{}.default_to_enabled()};

    std::string output_dir;
    std::string output;

    auto client_env_value(std::string const& key) const -> std::string
    {
        auto const client_pid = external_client.launch({"bash", "-c", ("echo ${" + key + ":-(unset)} >" + output).c_str()});
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
        auto const lp_fake_runtime_dir = strcmp(getenv("XDG_RUNTIME_DIR"), "/tmp") == 0;
        auto const cannot_access_x11_unix = access("/tmp/.X11-unix/", W_OK) != 0;

        return lp_fake_runtime_dir || cannot_access_x11_unix;
    }
};

auto const app_env = "MIR_SERVER_APP_ENV";
auto const app_x11_env = "MIR_SERVER_APP_ENV_X11";
}

TEST_F(ExternalClient, default_app_env_is_as_expected)
{
    start_server();

    EXPECT_THAT(client_env_value("GDK_BACKEND"), StrEq("wayland,x11"));
    EXPECT_THAT(client_env_value("QT_QPA_PLATFORM"), StrEq("wayland"));
    EXPECT_THAT(client_env_value("SDL_VIDEODRIVER"), StrEq("wayland"));
    EXPECT_THAT(client_env_value("NO_AT_BRIDGE"), StrEq("1"));
    EXPECT_THAT(client_env_value("_JAVA_AWT_WM_NONREPARENTING"), StrEq("1"));
    EXPECT_THAT(client_env_value("MOZ_ENABLE_WAYLAND"), StrEq("1"));
    EXPECT_THAT(client_env_value("GTK_MODULES"), StrEq("(unset)"));
    EXPECT_THAT(client_env_value("OOO_FORCE_DESKTOP"), StrEq("(unset)"));
    EXPECT_THAT(client_env_value("GNOME_ACCESSIBILITY"), StrEq("(unset)"));
}

TEST_F(ExternalClient, default_app_env_x11_is_as_expected)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_server_init(x11_disabled_by_default);
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
    add_to_environment(app_env, "GDK_BACKEND=mir");
    start_server();

    EXPECT_THAT(client_env_value("GDK_BACKEND"), StrEq("mir"));
}

TEST_F(ExternalClient, override_app_env_x11_can_unset)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, "-GDK_BACKEND");
    add_server_init(x11_disabled_by_default);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq(""));
}

TEST_F(ExternalClient, override_app_env_x11_can_unset_and_set)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_to_environment(app_x11_env, "-GDK_BACKEND:QT_QPA_PLATFORM=xcb");
    add_server_init(x11_disabled_by_default);
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
    add_server_init(x11_disabled_by_default);
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
    add_server_init(x11_disabled_by_default);
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
    add_server_init(x11_disabled_by_default);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq("wayland,x11"));
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
    add_server_init(x11_disabled_by_default);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq("wayland,x11"));
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
    add_server_init(x11_disabled_by_default);
    add_to_environment("MIR_SERVER_ENABLE_X11", "");
    start_server();

    EXPECT_THAT(client_env_x11_value("GDK_BACKEND"), StrEq("wayland,x11"));
    EXPECT_THAT(client_env_x11_value("QT_QPA_PLATFORM"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("SDL_VIDEODRIVER"), StrEq("wayland"));
    EXPECT_THAT(client_env_x11_value("NO_AT_BRIDGE"), StrEq("1"));
    EXPECT_THAT(client_env_x11_value("_JAVA_AWT_WM_NONREPARENTING"), StrEq("1"));
}

TEST_F(ExternalClient, split_command)
{
    struct { std::string command; std::vector<std::string> command_line; } const test_cases[] =
        {
            { "bash", {"bash"}},
            { "bash -c 'exit 1'", {"bash", "-c", "exit 1"}},
            { "bash -c \"exit 1\"", {"bash", "-c", "exit 1"}},
            { "wofi --show drun --location top_left", {"wofi", "--show", "drun", "--location", "top_left"}},
            { "swaybg -i \"/usr/share/backgrounds/warty-final-ubuntu.png\"",
              {"swaybg", "-i", "/usr/share/backgrounds/warty-final-ubuntu.png"}},
        };

    for (auto const& test : test_cases)
    {
        EXPECT_THAT(miral::ExternalClientLauncher::split_command(test.command), Eq(test.command_line))
            << "test.command=\"" + test.command + "\"";
    }
}

TEST_F(ExternalClient, x11_disabled_by_default_is_as_expected)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_server_init(x11_disabled_by_default);
    start_server();

    auto result = external_client.launch_using_x11({"bash", "-c", "exit"});
    EXPECT_THAT(result, Eq(-1));
}

TEST_F(ExternalClient, x11_enabled_by_default_is_as_expected)
{
    if (cannot_start_X_server())
        return; // Starting an X server doesn't work - skip the test

    add_server_init(x11_enabled_by_default);
    start_server();

    auto result = external_client.launch_using_x11({"bash", "-c", "exit"});
    EXPECT_THAT(result, Ne(-1));
}
