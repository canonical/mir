/*
 * Copyright © 2014-2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir_test_framework/async_server_runner.h"
#include "mir/test/popen.h"
#include "mir/test/spin_wait.h"
#include <mir_test_framework/executable_path.h>
#include <mir_test_framework/main.h>
#include <miral/x11_support.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

namespace mo = mir::options;
namespace mt = mir::test;
namespace mtf = mir_test_framework;
using namespace std::literals::chrono_literals;
using namespace testing;
using mir::test::spin_wait_for_condition_or_timeout;

namespace
{
struct AbstractGLMark2Test : testing::Test, mtf::AsyncServerRunner {
    void SetUp() override {
        start_server();
    }

    void TearDown() override {
        stop_server();
    }

    enum ResultFileType {
        raw, json
    };

    virtual char const *command() =0;

    int run_glmark2(char const *args) {
        ResultFileType file_type = raw; // Should this still be selectable?

        auto const cmd = std::string{} + command() + " -b build " + args;
        mir::test::Popen p(cmd);

        const ::testing::TestInfo *const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();

        char output_filename[256];
        snprintf(output_filename, sizeof(output_filename) - 1,
                 "/tmp/%s_%s.log",
                 test_info->test_case_name(), test_info->name());

        printf("Saving GLMark2 detailed results to: %s\n", output_filename);
        // ^ Although I would vote to just print them to stdout instead

        std::string line;
        std::ofstream glmark2_output;
        int score = -1;
        glmark2_output.open(output_filename);
        while (p.get_line(line)) {
            int match;
            if (sscanf(line.c_str(), " glmark2 Score: %d", &match) == 1) {
                score = match;
            }

            if (file_type == raw) {
                glmark2_output << line << std::endl;
            }
        }

        if (file_type == json) {
            std::string json = "{";
            json += "\"benchmark_name\":\"glmark2-es2-mir\"";
            json += ",";
            json += "\"score\":\"" + std::to_string(score) + "\"";
            json += "}";
            glmark2_output << json;
        }

        // Use GTest's structured annotation support to expose the score
        // to the test runner.
        RecordProperty("score", score);
        return score;
    }
};

struct GLMark2Xwayland : AbstractGLMark2Test
{
    GLMark2Xwayland()
    {
        // This is a slightly awkward method of enabling X11 support,
        // but refactoring these tests to use MirAL can wait.
        miral::X11Support{}(server);
        add_to_environment("MIR_SERVER_ENABLE_X11", "1");
    }

    char const* command() override
    {
        add_to_environment("DISPLAY", server.x11_display().value().c_str());
        static char const* const command = "glmark2-es2 --visual-config \"alpha=0\"";
        return command;
    }
};

struct GLMark2Wayland : AbstractGLMark2Test
{
    void SetUp() override
    {
        add_to_environment("WAYLAND_DISPLAY", "GLMark2Wayland");
        AbstractGLMark2Test::SetUp();
    }

    char const* command() override
    {
        static char const* const command = "glmark2-es2-wayland --visual-config \"alpha=0\"";
        return command;
    }
};

struct HostedGLMark2Wayland : GLMark2Wayland
{
    static char const* const host_socket;
    pid_t pid = 0;

    HostedGLMark2Wayland()
    {
        if ((pid = fork()))
        {
            EXPECT_THAT(pid, Gt(0));
        }
        else
        {
            auto args = get_host_args();

            add_to_environment("WAYLAND_DISPLAY", host_socket);
            std::string const server_path{mtf::executable_path() + "/mir_demo_server"};
            args[0] = server_path.c_str();

            const ::testing::TestInfo *const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();

            char output_filename[256];
            snprintf(output_filename, sizeof(output_filename) - 1,
                     "/tmp/%s_%s_host.log",
                     test_info->test_case_name(), test_info->name());

            printf("Saving host output to: %s\n", output_filename);
            if (freopen(output_filename, "a", stdout)) { /* (void)freopen(...); doesn't work */ };
            if (freopen(output_filename, "a", stderr)) { /* (void)freopen(...); doesn't work */ };

            execv(server_path.c_str(), const_cast<char* const*>(args.data()));
        }
    }

    ~HostedGLMark2Wayland()
    {
        if (pid > 0)
        {
            waitpid(pid, nullptr, 0);
        }
    }

    void SetUp() override
    {
        auto const socket = getenv("XDG_RUNTIME_DIR") + std::string("/") + host_socket;
        auto wait_for_host = [host = socket.c_str()] { return access(host, R_OK | W_OK) == 0; };
        EXPECT_TRUE(spin_wait_for_condition_or_timeout(wait_for_host, 10s));

        add_to_environment("MIR_SERVER_WAYLAND_HOST", host_socket);

        auto args = get_nested_args();
        server.set_command_line(args.size(), args.data());
        GLMark2Wayland::SetUp();
    }

    void TearDown() override
    {
        GLMark2Wayland::TearDown();
        if (pid > 0)
        {
            kill(pid, SIGTERM);
        }
    }

    std::vector<char const*> get_host_args() const;
    std::vector<char const*> get_nested_args() const;
};

char const* const HostedGLMark2Wayland::host_socket = "GLMark2WaylandHost";

std::vector<char const*> HostedGLMark2Wayland::get_host_args() const
{
    int argc;
    char const* const* argv;
    mtf::get_commandline(&argc, &argv);

    std::vector<char const*> args;

    for (auto arg = argv; arg != argv+argc; ++arg)
    {
        if (std::string("--logging") == *arg)
        {
            if (++arg == argv+argc)
                break;  // Lousy args, these will break anyway
        }
        else
        {
            args.push_back(*arg);
        }
    }

    args.push_back(nullptr);
    return args;
}

std::vector<char const*> HostedGLMark2Wayland::get_nested_args() const
{
    int argc;
    char const* const* argv;
    mtf::get_commandline(&argc, &argv);

    std::vector<char const*> args;

    auto arg = argv;

    args.push_back(*arg++);

    for (; arg != argv+argc; ++arg)
    {
        if (std::string("--logging") == *arg)
        {
            args.push_back(*arg);
            if (++arg == argv+argc)
                break;  // Lousy args, these will break anyway
            args.push_back(*arg);
        }
    }

    return args;
}

TEST_F(GLMark2Wayland, fullscreen)
{
    EXPECT_GT(run_glmark2("--fullscreen"), 0);
}

TEST_F(GLMark2Wayland, windowed)
{
    EXPECT_GT(run_glmark2(""), 0);
}

TEST_F(GLMark2Xwayland, fullscreen)
{
    EXPECT_GT(run_glmark2("--fullscreen"), 0);
}

TEST_F(GLMark2Xwayland, windowed)
{
    EXPECT_GT(run_glmark2(""), 0);
}

TEST_F(HostedGLMark2Wayland, fullscreen)
{
    EXPECT_GT(run_glmark2("--fullscreen"), 0);
}

TEST_F(HostedGLMark2Wayland, windowed)
{
    EXPECT_GT(run_glmark2(""), 0);
}
}
