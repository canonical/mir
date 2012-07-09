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

#ifndef MIR_POSIX_PROCESS_H_
#define MIR_POSIX_PROCESS_H_

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <stdexcept>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace mir
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

    enum class Signal : int
    {
        unknown = _NSIG+1,
        hangup = SIGHUP,
        interrupt = SIGINT,
        quit = SIGQUIT,
        illegal_instruction = SIGILL,
        trace_trap = SIGTRAP,
        abort = SIGABRT,
        iot_trap = SIGIOT,
        bus_error = SIGBUS,
        floating_point_exception = SIGFPE,
        kill = SIGKILL,
        user1 = SIGUSR1,
        user2 = SIGUSR2,
        segmentation_violation = SIGSEGV,
        broken_pipe = SIGPIPE,
        alarm_clock = SIGALRM,
        terminate = SIGTERM,
        child_status_changed = SIGCHLD,
        cont = SIGCONT,
        stop = SIGSTOP
    };
    
    struct Result
    {
        Result() : reason(TerminationReason::unknown),
                   exit_code(ExitCode::failure),
                   signal(Signal::unknown)
        {
        }
        
        bool is_successful() const
        {
            return reason == TerminationReason::child_terminated_normally &&
                    exit_code == ExitCode::success;
                
        }
        
        TerminationReason reason;
        ExitCode exit_code;
        Signal signal;
    };
    
    Process(pid_t pid) : pid(pid)
    {
        assert(pid > 0);
    }

    ~Process()
    {
        terminate();
        wait_for_termination();
    }

    Result wait_for_termination()
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
        } else if (WIFSIGNALED(status))
        {
            result.reason = TerminationReason::child_terminated_by_signal;            
        }
        
        return result;
    }

    void send_signal(Signal s)
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

    void kill()
    {
        send_signal(Signal::kill);
    }

    void terminate()
    {
        send_signal(Signal::terminate);
    }

    void stop()
    {
        send_signal(Signal::stop);
    }

    void cont()
    {
        send_signal(Signal::cont);
    }

 protected:
    Process() = delete;
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    
 private:
    pid_t pid;
};

std::ostream& operator<<(std::ostream& out, Process::TerminationReason reason)
{
    switch (reason)
    {
        case Process::TerminationReason::unknown:
            out << "unknown";
            break;
        case Process::TerminationReason::child_terminated_normally:
            out << "child_terminated_normally";
            break;
        case Process::TerminationReason::child_terminated_by_signal:
            out << "child_terminated_by_signal";
            break;
        case Process::TerminationReason::child_terminated_with_core_dump:
            out << "child_terminated_with_core_dump";
            break;
        case Process::TerminationReason::child_stopped_by_signal:
            out << "child_stopped_by_signal";
            break;
        case Process::TerminationReason::child_resumed_by_signal:
            out << "child_resumed_by_signal";
            break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, Process::ExitCode code)
{
    switch (code)
    {
        case Process::ExitCode::success:
            out << "success";
            break;            
        case Process::ExitCode::failure:
            out << "failure";
            break;
        default:
            out << "unknown ExitCode";
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const Process::Result& result)
{
    out << "Process::Result(" << result.reason << ", " << result.exit_code << ")";
    return out;
}

template<typename Callable>
std::shared_ptr<Process> fork_and_run_in_a_different_process(Callable&& f)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Problem forking process");
    }

    if (pid == client_pid)
    {
        f();
        exit(::testing::Test::HasFailure() ? static_cast<int>(Process::ExitCode::failure) : static_cast<int>(Process::ExitCode::success));
    } 
    
    return std::shared_ptr<Process>(new Process(pid));
}

}
}

#endif // MIR_POSIX_PROCESS_H_
