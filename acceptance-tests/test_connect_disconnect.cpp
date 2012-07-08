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

#include "mir/display_server.h"
#include "mir/frontend/application.h"
#include "mir/frontend/communicator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>

#include <sys/types.h>
#include <sys/wait.h>

namespace mf = mir::frontend;

namespace {

struct StubCommunicator : public mf::Communicator
{
    StubCommunicator()
    {
    }

    void run()
    {
    }
};

template<typename State>
class StateObserver
{
 public:
    typedef State StateType;
    
    virtual ~StateObserver() {}

    virtual void on_state_transition(State old_state, State new_state) = 0;

 protected:
    StateObserver() = default;
    StateObserver(const StateObserver&) = delete;
    StateObserver& operator=(const StateObserver&) = delete;
};

struct ApplicationStateObserver : public StateObserver<mf::Application::State>
{ 
    MOCK_METHOD2(
        on_state_transition,
        void(mf::Application::State, mf::Application::State));
};

static const pid_t client_pid = 0;

struct Process
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

    enum class ExitCode : int
    {
        success = EXIT_SUCCESS,
        failure = EXIT_FAILURE
    };
    
    struct Result
    {
        Result() : reason(TerminationReason::unknown),
                   exit_code(ExitCode::failure)
        {
        }
        
        bool is_successful() const
        {
            return reason == TerminationReason::child_terminated_normally &&
                    exit_code == ExitCode::success;
                
        }
        
        TerminationReason reason;
        ExitCode exit_code;
    };
    
    Process(pid_t pid) : pid(pid)
    {
        assert(pid > 0 );
    }

    Result wait()
    {
        int status;
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
Process fork_and_run_in_a_different_process(Callable&& f)
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
    
    return Process(pid);
}

}

TEST(client_server_communication, client_connects_and_disconnects)
{
    auto server_bind_and_connect = []() -> void
    {
        SCOPED_TRACE("Server");
        mir::DisplayServer display_server(nullptr);
        display_server.run();
    };

    // Set expectations here.
    
    

    std::shared_ptr<mf::Communicator> communicator(
        new StubCommunicator());
    
    mir::frontend::Application application(communicator);

    auto client_connect_and_disconnects = [&]() -> void
    {
        SCOPED_TRACE("Client");
        ApplicationStateObserver state_observer;
        application.state_transition_signal().connect(
            boost::bind(
                &ApplicationStateObserver::on_state_transition,
                &state_observer,
                _1,
                _2));                
        EXPECT_NO_THROW(application.connect());
        EXPECT_CALL(
            state_observer,
            on_state_transition(
                mf::Application::State::disconnected,
                mf::Application::State::connected));
        EXPECT_NO_THROW(application.disconnect());
        EXPECT_CALL(
            state_observer,
            on_state_transition(
                mf::Application::State::connected,
                mf::Application::State::disconnected));
    };
        
    EXPECT_TRUE(
        fork_and_run_in_a_different_process(
            client_connect_and_disconnects).wait().is_successful());

    EXPECT_TRUE(
        fork_and_run_in_a_different_process(server_bind_and_connect).wait().is_successful());

}
