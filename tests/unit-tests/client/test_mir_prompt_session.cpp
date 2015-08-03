/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Nick Dedekind <nick.dedekind <nick.dedekind@canonical.com>
 */

#include "src/client/mir_prompt_session.h"
#include "src/client/mir_event_distributor.h"
#include "src/client/rpc/mir_display_server.h"

#include "mir/events/event_builders.h"

#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;
namespace mev = mir::events;
namespace mt = mir::test;

namespace google
{
namespace protobuf
{
class RpcController;
}
}

namespace
{

struct MockProtobufServer : mclr::DisplayServer
{
    MockProtobufServer() : mclr::DisplayServer(nullptr) {}
    MOCK_METHOD3(start_prompt_session,
                 void(
                      mir::protobuf::PromptSessionParameters const* /*request*/,
                      mir::protobuf::Void* /*response*/,
                      google::protobuf::Closure* /*done*/));

    MOCK_METHOD3(stop_prompt_session,
                 void(
                      mir::protobuf::Void const* /*request*/,
                      mir::protobuf::Void* /*response*/,
                      google::protobuf::Closure* /*done*/));
};

class StubProtobufServer : public mclr::DisplayServer
{
public:
    StubProtobufServer() : mclr::DisplayServer(nullptr) {}

    void start_prompt_session(
        mir::protobuf::PromptSessionParameters const* /*request*/,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override
    {
        if (server_thread.joinable())
            server_thread.join();
        server_thread = std::thread{
            [response, done, this]
            {
                response->clear_error();
                done->Run();
            }};
    }

    void stop_prompt_session(
        mir::protobuf::Void const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* done) override
    {
        if (server_thread.joinable())
            server_thread.join();
        server_thread = std::thread{[done, this] { done->Run(); }};
    }

    ~StubProtobufServer()
    {
        if (server_thread.joinable())
            server_thread.join();
    }

private:
    std::thread server_thread;
};

class MirPromptSessionTest : public testing::Test
{
public:
    MirPromptSessionTest()
    {
    }

    static void prompt_session_state_change(MirPromptSession*, MirPromptSessionState new_state, void* context)
    {
        MirPromptSessionTest* test = static_cast<MirPromptSessionTest*>(context);
        test->state_updated(new_state);
    }

    MOCK_METHOD1(state_updated, void(MirPromptSessionState));

    testing::NiceMock<MockProtobufServer> mock_server;
    StubProtobufServer stub_server;
    MirEventDistributor event_distributor;
};

struct MockCallback
{
    MOCK_METHOD2(call, void(void*, void*));
};

void mock_callback_func(MirPromptSession* prompt_session, void* context)
{
    auto mock_cb = static_cast<MockCallback*>(context);
    mock_cb->call(prompt_session, context);
}

void null_callback_func(MirPromptSession*, void*)
{
}

ACTION(RunClosure)
{
    arg2->Run();
}

}

TEST_F(MirPromptSessionTest, start_prompt_session)
{
    using namespace testing;

    EXPECT_CALL(mock_server,
                start_prompt_session(_,_,_))
        .WillOnce(RunClosure());

    MirPromptSession prompt_session{
        mock_server,
        mt::fake_shared(event_distributor)};
    prompt_session.start(__LINE__, null_callback_func, nullptr);
}

TEST_F(MirPromptSessionTest, stop_prompt_session)
{
    using namespace testing;

    EXPECT_CALL(mock_server,
                stop_prompt_session(_,_,_))
        .WillOnce(RunClosure());

    MirPromptSession prompt_session{
        mock_server,
        mt::fake_shared(event_distributor)};
    prompt_session.stop(null_callback_func, nullptr);
}

TEST_F(MirPromptSessionTest, executes_callback_on_start)
{
    using namespace testing;

    MockCallback mock_cb;
    EXPECT_CALL(mock_cb, call(_, &mock_cb));

    MirPromptSession prompt_session{
        stub_server,
        mt::fake_shared(event_distributor)};
    prompt_session.start(__LINE__, mock_callback_func, &mock_cb)->wait_for_all();
}

TEST_F(MirPromptSessionTest, executes_callback_on_stop)
{
    using namespace testing;

    MockCallback mock_cb;
    EXPECT_CALL(mock_cb, call(_, &mock_cb));

    MirPromptSession prompt_session{
        stub_server,
        mt::fake_shared(event_distributor)};
    prompt_session.stop(mock_callback_func, &mock_cb)->wait_for_all();
}

TEST_F(MirPromptSessionTest, notifies_event_callback)
{
    using namespace testing;

    MirPromptSession prompt_session{
        mock_server,
        mt::fake_shared(event_distributor)};
    prompt_session.register_prompt_session_state_change_callback(&MirPromptSessionTest::prompt_session_state_change, this);

    InSequence seq;
    EXPECT_CALL(*this, state_updated(mir_prompt_session_state_started));
    EXPECT_CALL(*this, state_updated(mir_prompt_session_state_stopped));

    event_distributor.handle_event(*mev::make_event(mir_prompt_session_state_started));
    event_distributor.handle_event(*mev::make_event(mir_prompt_session_state_stopped));
}

