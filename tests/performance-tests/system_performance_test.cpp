/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "system_performance_test.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <string>
#include <thread>

using namespace std::literals::chrono_literals;

namespace
{

std::string mir_bin_dir()
{
    char path[256];
    auto len = readlink("/proc/self/exe", path, sizeof(path)-1);
    if (len < 0)
        len = 0;
    path[len] = '\0';
    if (auto slash = strrchr(path, '/'))
        *slash = '\0';
    return path;
}

void kill_nicely(pid_t pid)
{
    if (kill(pid, SIGTERM) < 0)
        return;
    int const timeout = 5;
    int status, count = 0;
    while (0 == waitpid(pid, &status, WNOHANG) && count < timeout)
    {
        sleep(1);
        ++count;
    }
    kill(pid, SIGKILL);
}

int exec_cmd(char const* cmd)
{
    auto buf = strdup(cmd);
    size_t argc = 1;
    char* argv[256] = {buf};
    char *c = buf;
    while (*c)
    {
        if (*c == ' ')
        {
            *c = '\0';
            if (argc < (sizeof(argv)/sizeof(argv[0]) - 1))
            {
                argv[argc] = c + 1;
                ++argc;
            }
        }
        ++c;
    }
    argv[argc] = NULL;
    return execv(argv[0], argv);
}

FILE* popen_with_pid(char const* cmd, pid_t& pid)
{
    int pipefd[2];
    if (pipe(pipefd))
        return NULL;

    int const& pipe_out = pipefd[0];
    int const& pipe_in = pipefd[1];

    pid = fork();
    if (pid < 0)
    {
        close(pipe_in);
        close(pipe_out);
        return NULL;
    }
    else if (pid == 0)
    {
        close(pipe_out);
        dup2(pipe_in, 1);  // Child stdout goes into pipe_in
        close(pipe_in);
        exec_cmd(cmd);
        exit(errno);
    }
    else
    {
        close(pipe_in);
    }

    return fdopen(pipe_out, "r");
}

bool spawn_and_forget(char const* cmd)
{
    int pid = fork();
    if (pid == 0)
    {
        // Silence stdout/stderr
        close(1);
        close(2);
        exec_cmd(cmd);
        exit(errno);
    }
    return (pid > 0);
}

bool spawn_and_forget(std::string const& cmd)
{
    return spawn_and_forget(cmd.c_str());
}

bool wait_for_file(char const* path, std::chrono::seconds timeout)
{
    struct stat s;
    int count = 0, max = timeout.count();
    int ret;
    while ((ret = stat(path, &s)) < 0 && errno == ENOENT && count < max)
    {
        ++count;
        std::this_thread::sleep_for(1s);
    }
    return ret == 0;
}

} // anonymous namespace

namespace mir { namespace test {

SystemPerformanceTest::SystemPerformanceTest() : bin_dir{mir_bin_dir()}
{
}

void SystemPerformanceTest::set_up_with(std::string const server_args)
{
    auto const mir_sock = "/tmp/mir_test_socket_"+std::to_string(getpid());
    auto const server_cmd =
        bin_dir+"/mir_demo_server -f "+mir_sock+" "+server_args;

    server_output = popen_with_pid(server_cmd.c_str(), server_pid);
    ASSERT_TRUE(server_output) << server_cmd;
    ASSERT_TRUE(wait_for_file(mir_sock.c_str(), 5s)) << server_cmd;
    setenv("MIR_SOCKET", mir_sock.c_str(), 1);
}

void SystemPerformanceTest::TearDown()
{
    kill_nicely(server_pid);
    fclose(server_output);
}

void SystemPerformanceTest::spawn_clients(std::initializer_list<std::string> clients)
{
    for (auto& client : clients)
    {
        spawn_and_forget(bin_dir+"/"+client);
        std::this_thread::sleep_for(100ms);
    }
}

void SystemPerformanceTest::run_server_for(std::chrono::seconds timeout)
{
    pid_t pid = server_pid;
    std::thread killer([timeout,pid]()
    {
        std::this_thread::sleep_for(timeout);
        kill_nicely(pid);
    });
    killer.detach();
}

} } // namespace mir::test
