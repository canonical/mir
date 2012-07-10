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

#include "mir/process/posix_process.h"

#include <functional>
#include <memory>

namespace mpx = mir::process::posix;

mpx::Process::Result::Result()
  : reason(TerminationReason::unknown),
    exit_code(ExitCode::failure),
    signal(Signal::unknown)
{
}

bool mpx::Process::Result::is_successful() const
{
    return reason == TerminationReason::child_terminated_normally &&
        exit_code == ExitCode::success;
}

mpx::Process::Process(pid_t pid)
  : pid(pid)
{
    assert(pid > 0);
}

mpx::Process::~Process()
{
    terminate();
    wait_for_termination();
}

mpx::Process::Result mpx::Process::wait_for_termination()
{
    int status;
    // ToDo handle errors here
    waitpid(pid, &status, WUNTRACED | WCONTINUED);

    Result result;
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
    }
    return result;
}

void mpx::Process::send_signal(Signal s)
{
    int result = ::kill(pid, static_cast<int>(s));

    switch (result)
    {
    case EINVAL:
        throw InvalidSignalException();
        break;
    case EPERM:
        throw ProcessPermissionException();
        break;
    case ESRCH:
        throw ProcessDoesNotExistException();
        break;
    }
}

void mpx::Process::kill()
{
    send_signal(Signal::kill);
}

void mpx::Process::terminate()
{
    send_signal(Signal::terminate);
}

void mpx::Process::stop()
{
    send_signal(Signal::stop);
}

void mpx::Process::cont()
{
    send_signal(Signal::cont);
}

std::ostream& operator<<(std::ostream& out, mpx::Process::TerminationReason reason)
{
    switch (reason)
    {
    case mpx::Process::TerminationReason::unknown:
        out << "unknown";
        break;
    case mpx::Process::TerminationReason::child_terminated_normally:
        out << "child_terminated_normally";
        break;
    case mpx::Process::TerminationReason::child_terminated_by_signal:
        out << "child_terminated_by_signal";
        break;
    case mpx::Process::TerminationReason::child_terminated_with_core_dump:
        out << "child_terminated_with_core_dump";
        break;
    case mpx::Process::TerminationReason::child_stopped_by_signal:
        out << "child_stopped_by_signal";
        break;
    case mpx::Process::TerminationReason::child_resumed_by_signal:
        out << "child_resumed_by_signal";
        break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, mpx::Process::ExitCode code)
{
    switch (code)
    {
    case mpx::Process::ExitCode::success:
        out << "success";
        break;
    case mpx::Process::ExitCode::failure:
        out << "failure";
        break;
    default:
        out << "unknown ExitCode";
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const mpx::Process::Result& result)
{
    out << "Process::Result(" << result.reason << ", " << result.exit_code << ")";
    return out;
}

template<typename Callable>
std::shared_ptr<mpx::Process> fork_and_run_in_a_different_process(Callable&& f)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw mpx::ProcessForkError();
    }

    if (pid == mpx::client_pid)
    {
        f();
        exit(::testing::Test::HasFailure()
             ? static_cast<int>(mpx::Process::ExitCode::failure)
             : static_cast<int>(mpx::Process::ExitCode::success));
    }

    return std::shared_ptr<mpx::Process>(new mpx::Process(pid));
}
