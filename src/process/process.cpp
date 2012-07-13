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

mp::Process::Result::Result()
    : reason(TerminationReason::unknown)
    , exit_code(ExitCode::failure)
    , signal(_NSIG+1)
{
}

bool mp::Process::Result::succeeded() const
{
    return reason == TerminationReason::child_terminated_normally &&
        exit_code == ExitCode::success;
}

bool mp::Process::Result::signalled() const
{
    return reason == TerminationReason::child_terminated_by_signal;
}

mp::Process::Process(pid_t pid)
    : pid(pid)
    , terminated(false)
{
    assert(pid > 0);
}

mp::Process::~Process()
{
    if (!terminated)
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

mp::Process::Result mp::Process::wait_for_termination()
{
    Result result;
    int status;
    terminated = ::waitpid(pid, &status, WUNTRACED | WCONTINUED) != -1;
    
    if (terminated)
    {
        if (WIFEXITED(status))
        {
            result.reason = TerminationReason::child_terminated_normally;

            switch (WEXITSTATUS(status))
            {
            case EXIT_SUCCESS:
                result.exit_code = ExitCode::success;
                break;
            case EXIT_FAILURE:
                result.exit_code = ExitCode::failure;
                break;
            }
        }
        else if (WIFSIGNALED(status))
        {
            result.reason = TerminationReason::child_terminated_by_signal;
            result.signal = WTERMSIG(status);
        }
    }
    return result;
}

void mp::Process::kill()
{
    signal_process(pid, SIGKILL);
}

void mp::Process::terminate()
{
    signal_process(pid, SIGTERM);
}

void mp::Process::stop()
{
    signal_process(pid, SIGSTOP);
}

void mp::Process::cont()
{
    signal_process(pid, SIGCONT);
}

std::ostream& operator<<(std::ostream& out, mp::Process::TerminationReason reason)
{
    switch (reason)
    {
    case mp::Process::TerminationReason::unknown:
        out << "unknown";
        break;
    case mp::Process::TerminationReason::child_terminated_normally:
        out << "child_terminated_normally";
        break;
    case mp::Process::TerminationReason::child_terminated_by_signal:
        out << "child_terminated_by_signal";
        break;
    case mp::Process::TerminationReason::child_terminated_with_core_dump:
        out << "child_terminated_with_core_dump";
        break;
    case mp::Process::TerminationReason::child_stopped_by_signal:
        out << "child_stopped_by_signal";
        break;
    case mp::Process::TerminationReason::child_resumed_by_signal:
        out << "child_resumed_by_signal";
        break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, mp::Process::ExitCode code)
{
    switch (code)
    {
    case mp::Process::ExitCode::success:
        out << "success";
        break;
    case mp::Process::ExitCode::failure:
        out << "failure";
        break;
    default:
        out << "unknown ExitCode";
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const mp::Process::Result& result)
{
    out << "Process::Result(" << result.reason << ", " << result.exit_code << ")";
    return out;
}
