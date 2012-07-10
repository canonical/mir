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
#include "mir/process/posix_process.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace mf = mir::frontend;
namespace mpx = mir::process::posix;

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
}

int test_exit()
{
    return ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
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
    std::shared_ptr<mpx::Process> server =
            mpx::fork_and_run_in_a_different_process(
                server_bind_and_connect, test_exit);

    std::shared_ptr<mf::Communicator> communicator(
        new StubCommunicator());

    mir::frontend::Application application(communicator);

    auto client_connects_and_disconnects = [&]() -> void
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

    std::shared_ptr<mpx::Process> client =
            mpx::fork_and_run_in_a_different_process(
                client_connects_and_disconnects, test_exit);

    EXPECT_TRUE(client->wait_for_termination().is_successful());

    server->terminate();
    EXPECT_TRUE(server->wait_for_termination().is_successful());
}
