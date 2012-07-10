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
 */

#ifndef MIR_PROCESS_POSIX_PROCESS_H_
#define MIR_PROCESS_POSIX_PROCESS_H_

#include "posix_signal.h"

#include <memory>
#include <iosfwd>
#include <stdexcept>

namespace mir
{
namespace process
{
namespace posix
{

static const pid_t client_pid = 0;

struct InvalidSignalException : public std::runtime_error
{
    InvalidSignalException() : std::runtime_error("Unknown signal")
    {
    }
};

struct ProcessPermissionException : public std::runtime_error
{
    ProcessPermissionException() : std::runtime_error("Missing permissions to alter process")
    {
    }
};

struct ProcessDoesNotExistException : public std::runtime_error
{
    ProcessDoesNotExistException() : std::runtime_error("No such process or process group")
    {
    }
};

struct ProcessForkError : public std::runtime_error
{
    ProcessForkError() : std::runtime_error("Failed to fork process")
    {
    }
};

// Posix process control class.
class Process
{
public:
    enum class TerminationReason
    {
        unknown,
        child_terminated_normally,
        child_terminated_by_signal,
        child_terminated_with_core_dump,
        child_stopped_by_signal,
        child_resumed_by_signal
    };

    enum class ExitCode : int
    {
        success = EXIT_SUCCESS,
        failure = EXIT_FAILURE
    };

    // Aggregated results of running a process to completion
    struct Result
    {
        Result();

        // Did the process terminate without error?
        bool is_successful() const;

        TerminationReason reason;
        ExitCode exit_code;
        Signal signal;
    };

    // Construct a process with the supplied pid
    Process(pid_t pid);

    // Destroy the process cleanly, by sending it the termination signal
    // and waiting for the pid.
    ~Process();

    // Wait for the process to terminate, and return the results.
    Result wait_for_termination();

    // Attempt to kill the process by sending the supplied signal.
    // A failure will result in an exception being thrown.
    void send_signal(Signal s);

    // Kill, terminate or stop the process by signalling it.
    void kill();
    void terminate();
    void stop();

    // Continue the process.
    void cont();

protected:
    Process() = delete;
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

private:
    pid_t pid;
};

// Stream print helper
std::ostream& operator<<(std::ostream& out, const Process::Result& result);

// Fork a child process to run the supplied main function, calling
// the exit function when done.
// Returns the parent process.
template<typename Callable>
std::shared_ptr<Process> fork_and_run_in_a_different_process(
    Callable&& main_fn, int (exit_fn)())
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw ProcessForkError();
    }

    if (pid == client_pid)
    {
        main_fn();
        exit(exit_fn());
    }

    return std::shared_ptr<Process>(new Process(pid));
}

}
}
}

#endif // MIR_PROCESS_POSIX_PROCESS_H_
