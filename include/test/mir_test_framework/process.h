/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Thomas Guest <thomas.guest@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_PROCESS_H_
#define MIR_TEST_FRAMEWORK_PROCESS_H_

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <memory>
#include <stdexcept>

#include <unistd.h>

namespace mir_test_framework
{

enum class TerminationReason
{
    unknown,
    child_terminated_normally,
    child_terminated_by_signal,
    child_terminated_with_core_dump,
    child_stopped_by_signal,
    child_resumed_by_signal
};


// Aggregated results of running a process to completion
struct Result
{
    Result();

    // Did the process exit without error?
    bool succeeded() const;

    // Was the process terminated by a signal?
    bool signalled() const;

    // Was the process terminated normally?
    bool exited_normally() const;

    TerminationReason reason;
    int exit_code;
    int signal;
};

// Posix process control class.
class Process
{
public:
    // Construct a process with the supplied pid
    Process(pid_t pid);

    // Destroy the process cleanly, by terminating it and waiting for
    // the pid.
    ~Process();

    // Wait for the process to terminate, and return the results.
    Result wait_for_termination(const std::chrono::milliseconds& timeout = std::chrono::minutes(4));

    void kill();
    void terminate();
    void stop();
    void cont();
    void detach();

protected:
    Process() = delete;
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

private:
    pid_t pid;
    bool terminated;
    bool detached;
};

// Stream print helper
std::ostream& operator<<(std::ostream& out, const Result& result);

// Fork a child process to run the supplied main function, calling
// the exit function when done.
// Returns the parent process.
template<typename Callable>
std::shared_ptr<Process> fork_and_run_in_a_different_process(
    Callable&& main_fn, std::function<int()> exit_fn)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        main_fn();
        exit(exit_fn());
    }

    return std::shared_ptr<Process>(new Process(pid));
}
}

#endif // MIR_TEST_FRAMEWORK_PROCESS_H_
