/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include <sys/wait.h>

#include <boost/exception/errinfo_errno.hpp>

#include <system_error>
#include <cassert>
#include <chrono>
#include <ostream>
#include <string>
#include <thread>

namespace mtf = mir_test_framework;

namespace
{
struct SignalNumberErrorInfoTag {};
typedef boost::error_info<SignalNumberErrorInfoTag, int> errinfo_signum;
struct ProcessIdErrorInfoTag {};
typedef boost::error_info<ProcessIdErrorInfoTag, pid_t> errinfo_pid;

void signal_process(pid_t pid, int signum)
{
    if (::kill(pid, signum) != 0)
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::system_error(errno,
                                                         std::system_category(),
                                                         "Failed to kill process."))
            << errinfo_pid(pid)
            << errinfo_signum(signum));
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

bool mtf::Result::exited_normally() const
{
    return reason == TerminationReason::child_terminated_normally;
}

mtf::Process::Process(pid_t pid)
    : pid(pid)
    , terminated(false)
    , detached(false)
{
    assert(pid > 0);
}

mtf::Process::~Process()
{
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
        auto tp = std::chrono::steady_clock::now() + timeout;
        int rc = -1;
        while (true)
        {
            if ((rc = ::waitpid(pid, &status, WNOHANG)) == pid)
            {
                if (WIFEXITED(status))
                {
                    terminated = true;
                    result.reason = TerminationReason::child_terminated_normally;
                    result.exit_code = WEXITSTATUS(status);
                    break;
                }
                else if (WIFSIGNALED(status))
                {
                    terminated = true;
                    result.reason = TerminationReason::child_terminated_by_signal;
                    result.signal = WTERMSIG(status);
                    break;
                }
                else {
                    terminated = true;
                    result.reason = TerminationReason::unknown;
                    break;
                }
            }
            else if (rc == 0)
            {
                if (std::chrono::steady_clock::now() < tp)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                else
                {
                    BOOST_THROW_EXCEPTION(
                        ::boost::enable_error_info(std::runtime_error("Timeout while waiting for child to change state"))
                        << errinfo_pid(pid));
                }
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
