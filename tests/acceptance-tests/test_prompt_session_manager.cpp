/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "src/server/scene/prompt_session_manager_impl.h"
#include "src/server/scene/session_container.h"

#include "mir/scene/prompt_session.h"
#include "mir/scene/prompt_session_manager.h"
#include "mir/scene/prompt_session_creation_parameters.h"

#include "mir_test_doubles/stub_scene_session.h"
#include "mir_test_doubles/mock_prompt_session_listener.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
struct StubSessionContainer : ms::SessionContainer
{
    void insert_session(std::shared_ptr<ms::Session> const& session)
    {
        sessions.push_back(session);
    }

    void remove_session(std::shared_ptr<ms::Session> const&)
    {
    }

    void for_each(std::function<void(std::shared_ptr<ms::Session> const&)> f) const
    {
        for (auto const& session : sessions)
            f(session);
    }

    std::shared_ptr<ms::Session> successor_of(std::shared_ptr<ms::Session> const&) const
    {
        return {};
    }

    std::vector<std::shared_ptr<ms::Session>> sessions;
};

struct PromptSessionManager : public testing::Test
{
    pid_t const application_pid = __LINE__;
    pid_t const helper_pid = __LINE__;
    pid_t const prompt_provider_pid = __LINE__;
    std::shared_ptr<ms::Session> const helper{std::make_shared<mtd::StubSceneSession>(helper_pid)};
    std::shared_ptr<ms::Session> const application_session{std::make_shared<mtd::StubSceneSession>(application_pid)};
    StubSessionContainer existing_sessions;

    NiceMock<mtd::MockPromptSessionListener> prompt_session_listener;

    ms::PromptSessionManagerImpl session_manager{mt::fake_shared(existing_sessions), mt::fake_shared(prompt_session_listener)};

    std::vector<std::shared_ptr<ms::Session>> list_providers_for(std::shared_ptr<ms::PromptSession> const& prompt_session)
    {
        std::vector<std::shared_ptr<ms::Session>> results;
        auto providers_fn = [&results](std::weak_ptr<ms::Session> const& session)
        {
            results.push_back(session.lock());
        };

        session_manager.for_each_provider_in(prompt_session, providers_fn);

        return results;
    }
};
}

TEST_F(PromptSessionManager, sets_application_for)
{
    existing_sessions.insert_session(application_session);

    ms::PromptSessionCreationParameters parameters;
    parameters.application_pid = application_pid;
    auto const prompt_session = session_manager.start_prompt_session_for(helper, parameters);

    EXPECT_EQ(session_manager.application_for(prompt_session), application_session);
}
