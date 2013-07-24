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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_test_framework/process.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <boost/exception/errinfo_errno.hpp>

#include <cassert>
#include <chrono>
#include <ostream>
#include <string>
#include <thread>

namespace mtf = mir_test_framework;

namespace
{
void signal_process(pid_t pid, int signum)
{
    std::cout << __PRETTY_FUNCTION__ << ": " << pid << ", " << signum << std::endl;
    if (::kill(pid, signum) != 0)
    {
        throw std::runtime_error(std::string("Failed to kill process: ")
                                 + ::strerror(errno));
    }
}
}

mtf::Result::Result()
    : reason(TerminationReason::unknown)
    , exit_code(EXIT_FAILURE)
    , signal(_NSIG+1)
{
}

bool mtf::Result::succeeded() const
{
    return reason == TerminationReason::child_terminated_normally &&
        exit_code == EXIT_SUCCESS;
}

bool mtf::Result::signalled() const
{
    return reason == TerminationReason::child_terminated_by_signal;
}

mtf::Process::Process(pid_t pid)
    : pid(pid)
    , terminated(false)
    , detached(false)
{
    assert(pid > 0);

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1)
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Error attaching to child process"))
                << (boost::errinfo_errno(errno)));

    if (waitpid (pid, NULL, 0) < 0 )
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Error waiting on child process"))
                << (boost::errinfo_errno(errno)));


    if (ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Error continuing child process"))
                << (boost::errinfo_errno(errno)));
}

mtf::Process::~Process()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!detached && !terminated)
    {
        try
        {
            terminate();
            wait_for_termination();
        }
        catch (std::exception const &)
        {
            // Ignore a failure to signal the process.
        }
    }
}

mtf::Result mtf::Process::wait_for_termination(const std::chrono::milliseconds& timeout)
{
    Result result;
    int status;

    if (!detached)
    {
        auto tp = std::chrono::system_clock::now() + timeout;
        int rc = -1;
        while (true)
        {
            if ((rc = ::waitpid(pid, &status, WNOHANG)) == pid)
            {                
                if (WIFEXITED(status))
                {
                    std::cout << "\t (WIFSIGNALED(status)): " << pid << std::endl;

                    terminated = true;
                    result.reason = TerminationReason::child_terminated_normally;
                    result.exit_code = WEXITSTATUS(status);
                    break;
                }
                else if (WIFSIGNALED(status))
                {
                    std::cout << "\t (WIFSIGNALED(status)): " << pid << std::endl;
                    terminated = true;
                    result.reason = TerminationReason::child_terminated_by_signal;
                    result.signal = WTERMSIG(status);
                    break;
                }
                else if (WIFSTOPPED(status))
                {
                    int stop_signal = WSTOPSIG(status);
                    std::cout << "\t WIFSTOPPED(status): " << pid << ", " << stop_signal << std::endl;

                    if (ptrace(PTRACE_CONT, pid, nullptr, stop_signal) == -1)
                    {
                        BOOST_THROW_EXCEPTION(
                            ::boost::enable_error_info(std::runtime_error("Error continuing child process"))
                            << (boost::errinfo_errno(errno)));
                    }
                }
                else if (WIFCONTINUED(status))
                {
                    std::cout << "\t (WIFSIGNALED(status)): " << std::endl;
                    continue;
                }
            }
            else if (rc == 0)
            {
                if (std::chrono::system_clock::now() < tp)
                {
                    std::this_thread::yield();
                    continue;
                }
                else
                    throw std::runtime_error("Timeout while waiting for child to change state");
            }
            else
                break;
        }
    }
    return result;
}

void mtf::Process::kill()
{
    if (!detached) signal_process(pid, SIGKILL);
}

void mtf::Process::terminate()
{
    if (!detached) signal_process(pid, SIGTERM);
}

void mtf::Process::stop()
{
    if (!detached) signal_process(pid, SIGSTOP);
}

void mtf::Process::cont()
{
    if (!detached) signal_process(pid, SIGCONT);
}

void mtf::Process::detach()
{
    detached = true;
}

namespace
{
std::ostream& print_reason(std::ostream & out, mtf::TerminationReason reason)
{
    switch (reason)
    {
    case mtf::TerminationReason::unknown:
        out << "unknown";
        break;
    case mtf::TerminationReason::child_terminated_normally:
        out << "child_terminated_normally";
        break;
    case mtf::TerminationReason::child_terminated_by_signal:
        out << "child_terminated_by_signal";
        break;
    case mtf::TerminationReason::child_terminated_with_core_dump:
        out << "child_terminated_with_core_dump";
        break;
    case mtf::TerminationReason::child_stopped_by_signal:
        out << "child_stopped_by_signal";
        break;
    case mtf::TerminationReason::child_resumed_by_signal:
        out << "child_resumed_by_signal";
        break;
    }

    return out;
}

std::ostream& print_signal(std::ostream& out, int signal)
{
    out << "signal(" << signal << ")";
    return out;
}

std::ostream& print_exit_code(std::ostream& out, int exit_code)
{
    if (exit_code == EXIT_SUCCESS)
    {
        out << "success";
    }
    else
    {
        out << "failure(" << exit_code << ')';
    }
    return out;
}
}

std::ostream& mtf::operator<<(std::ostream& out, const mtf::Result& result)
{
    out << "process::Result(";
    print_reason(out, result.reason);
    out << ", ";
    if (result.signalled())
    {
        print_signal(out, result.signal);
        out << ", ";
    }

    if (result.reason == TerminationReason::child_terminated_normally)
    {
        print_exit_code(out, result.exit_code);
    }

    return out << ')';
}
