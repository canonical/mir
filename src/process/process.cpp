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

#include "mir/process/process.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cassert>
#include <ostream>
#include <string>

namespace mp = mir::process;

namespace
{
void signal_process(pid_t pid, int signum)
{
    if (::kill(pid, signum) != 0)
    {
        throw std::runtime_error(std::string("Failed to kill process: ")
                                 + ::strerror(errno));
    }
}
}

mp::Result::Result()
    : reason(TerminationReason::unknown)
    , exit_code(EXIT_FAILURE)
    , signal(_NSIG+1)
{
}

bool mp::Result::succeeded() const
{
    return reason == TerminationReason::child_terminated_normally &&
        exit_code == EXIT_SUCCESS;
}

bool mp::Result::signalled() const
{
    return reason == TerminationReason::child_terminated_by_signal;
}

mp::Process::Process(pid_t pid)
    : pid(pid)
    , terminated(false)
    , detached(false)
{
    assert(pid > 0);
}

mp::Process::~Process()
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

mp::Result mp::Process::wait_for_termination()
{
    Result result;
    int status;

    if (!detached)
    {
        terminated = ::waitpid(pid, &status, WUNTRACED | WCONTINUED) != -1;

        if (terminated)
        {
            if (WIFEXITED(status))
            {
                result.reason = TerminationReason::child_terminated_normally;
                result.exit_code = WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status))
            {
                result.reason = TerminationReason::child_terminated_by_signal;
                result.signal = WTERMSIG(status);
            }
        }
    }
    return result;
}

void mp::Process::kill()
{
    if (!detached) signal_process(pid, SIGKILL);
}

void mp::Process::terminate()
{
    if (!detached) signal_process(pid, SIGTERM);
}

void mp::Process::stop()
{
    if (!detached) signal_process(pid, SIGSTOP);
}

void mp::Process::cont()
{
    if (!detached) signal_process(pid, SIGCONT);
}

void mp::Process::detach()
{
    detached = true;
}

namespace
{
std::ostream& print_reason(std::ostream & out, mp::TerminationReason reason)
{
    switch (reason)
    {
    case mp::TerminationReason::unknown:
        out << "unknown";
        break;
    case mp::TerminationReason::child_terminated_normally:
        out << "child_terminated_normally";
        break;
    case mp::TerminationReason::child_terminated_by_signal:
        out << "child_terminated_by_signal";
        break;
    case mp::TerminationReason::child_terminated_with_core_dump:
        out << "child_terminated_with_core_dump";
        break;
    case mp::TerminationReason::child_stopped_by_signal:
        out << "child_stopped_by_signal";
        break;
    case mp::TerminationReason::child_resumed_by_signal:
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

std::ostream& mir::process::operator<<(std::ostream& out, const mp::Result& result)
{
    out << "process::Result(";
    print_reason(out, result.reason);
    out << ", ";
    print_signal(out, result.signal);
    out << ", ";
    print_exit_code(out, result.exit_code) << ')';
    return out;
}
