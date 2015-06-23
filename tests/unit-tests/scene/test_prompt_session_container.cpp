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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "src/server/scene/prompt_session_container.h"
#include "mir/test/doubles/mock_scene_session.h"
#include "mir/test/doubles/null_prompt_session.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
struct PromptSessionContainer : testing::Test
{
    static constexpr pid_t process1 = 1;
    static constexpr pid_t process2 = 2;

    mtd::NullPromptSession null_prompt_session1;
    mtd::NullPromptSession null_prompt_session2;
    mtd::NullPromptSession null_prompt_session3;

    std::shared_ptr<ms::PromptSession> const prompt_session1 = mt::fake_shared(null_prompt_session1);
    std::shared_ptr<ms::PromptSession> const prompt_session2 = mt::fake_shared(null_prompt_session2);
    std::shared_ptr<ms::PromptSession> const prompt_session3 = mt::fake_shared(null_prompt_session3);

    testing::NiceMock<mtd::MockSceneSession> mock_scene_session1;
    testing::NiceMock<mtd::MockSceneSession> mock_scene_session2;
    testing::NiceMock<mtd::MockSceneSession> mock_scene_session3;
    testing::NiceMock<mtd::MockSceneSession> mock_scene_session4;

    std::shared_ptr<ms::Session> const session1 = mt::fake_shared(mock_scene_session1);
    std::shared_ptr<ms::Session> const session2 = mt::fake_shared(mock_scene_session2);
    std::shared_ptr<ms::Session> const session3 = mt::fake_shared(mock_scene_session3);
    std::shared_ptr<ms::Session> const session4 = mt::fake_shared(mock_scene_session4);

    ms::PromptSessionContainer container;

    void SetUp() {
        ON_CALL(mock_scene_session1, process_id()).WillByDefault(Return(process1));
        ON_CALL(mock_scene_session2, process_id()).WillByDefault(Return(process1));
        ON_CALL(mock_scene_session3, process_id()).WillByDefault(Return(process2));
        ON_CALL(mock_scene_session4, process_id()).WillByDefault(Return(process2));
    }

    std::vector<std::shared_ptr<ms::Session>> list_participants_for(std::shared_ptr<ms::PromptSession> const& prompt_session)
    {
        std::vector<std::shared_ptr<ms::Session>> results;
        auto list_participants = [&results](std::weak_ptr<ms::Session> const& session, ms::PromptSessionContainer::ParticipantType)
        {
            results.push_back(session.lock());
        };

        container.for_each_participant_in_prompt_session(prompt_session.get(), list_participants);

        return results;
    }

    int count_participants_for(std::shared_ptr<ms::PromptSession> const& prompt_session)
    {
        return list_participants_for(prompt_session).size();
    }

    std::vector<std::shared_ptr<ms::PromptSession>> list_prompt_sessions_for(std::shared_ptr<ms::Session> const& session)
    {
        std::vector<std::shared_ptr<ms::PromptSession>> results;
        auto list_prompt_sessions = [&results](std::shared_ptr<ms::PromptSession> const& prompt_session, ms::PromptSessionContainer::ParticipantType)
        {
            results.push_back(prompt_session);
        };

        container.for_each_prompt_session_with_participant(session, list_prompt_sessions);

        return results;
    }

    std::vector<std::shared_ptr<ms::PromptSession>> list_prompt_sessions_for(std::shared_ptr<ms::Session> const& session, ms::PromptSessionContainer::ParticipantType participant_type)
    {
        std::vector<std::shared_ptr<ms::PromptSession>> results;
        auto list_prompt_sessions = [&results](std::shared_ptr<ms::PromptSession> const& prompt_session)
        {
            results.push_back(prompt_session);
        };

        container.for_each_prompt_session_with_participant(session, participant_type, list_prompt_sessions);

        return results;
    }

    int count_prompt_sessions_for(std::shared_ptr<ms::Session> const& session)
    {
        return list_prompt_sessions_for(session).size();
    }
};
}

TEST_F(PromptSessionContainer, throws_exception_if_no_prompt_session)
{
    EXPECT_THROW(
        container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider),
        std::runtime_error);
}

TEST_F(PromptSessionContainer, insert_true_if_prompt_session_exists)
{
    container.insert_prompt_session(prompt_session1);
    EXPECT_TRUE(container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider));
}

TEST_F(PromptSessionContainer, insert_true_if_not_duplicate)
{
    container.insert_prompt_session(prompt_session1);
    container.insert_prompt_session(prompt_session2);
    container.insert_prompt_session(prompt_session3);

    EXPECT_TRUE(container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider));
    EXPECT_TRUE(container.insert_participant(prompt_session1.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider));
    EXPECT_TRUE(container.insert_participant(prompt_session1.get(), session3, ms::PromptSessionContainer::ParticipantType::prompt_provider));
    EXPECT_FALSE(container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider));
    EXPECT_TRUE(container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::helper));
    EXPECT_TRUE(container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::application));

    EXPECT_TRUE(container.insert_participant(prompt_session2.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider));
    EXPECT_TRUE(container.insert_participant(prompt_session2.get(), session3, ms::PromptSessionContainer::ParticipantType::prompt_provider));
    EXPECT_FALSE(container.insert_participant(prompt_session2.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider));

    EXPECT_TRUE(container.insert_participant(prompt_session3.get(), session3, ms::PromptSessionContainer::ParticipantType::prompt_provider));
    EXPECT_FALSE(container.insert_participant(prompt_session3.get(), session3, ms::PromptSessionContainer::ParticipantType::prompt_provider));
}

TEST_F(PromptSessionContainer, lists_participants_in_a_prompt_session)
{
    container.insert_prompt_session(prompt_session1);
    container.insert_prompt_session(prompt_session2);
    container.insert_prompt_session(prompt_session3);

    container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session1.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session2.get(), session3, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session2.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session3.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session2.get(), session4, ms::PromptSessionContainer::ParticipantType::prompt_provider);

    EXPECT_THAT(list_participants_for(prompt_session1), ElementsAre(session1, session2));
    EXPECT_THAT(list_participants_for(prompt_session2), ElementsAre(session3, session1, session4));
    EXPECT_THAT(list_participants_for(prompt_session3), ElementsAre(session2));
}

TEST_F(PromptSessionContainer, lists_prompt_sessions_for_a_participant)
{
    container.insert_prompt_session(prompt_session1);
    container.insert_prompt_session(prompt_session2);
    container.insert_prompt_session(prompt_session3);

    container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::helper);
    container.insert_participant(prompt_session1.get(), session2, ms::PromptSessionContainer::ParticipantType::application);
    container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session1.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session2.get(), session3, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session2.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session3.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session2.get(), session4, ms::PromptSessionContainer::ParticipantType::prompt_provider);

    EXPECT_THAT(list_prompt_sessions_for(session1), ElementsAre(prompt_session1, prompt_session1, prompt_session2));
    EXPECT_THAT(list_prompt_sessions_for(session1, ms::PromptSessionContainer::ParticipantType::helper), ElementsAre(prompt_session1));
    EXPECT_THAT(list_prompt_sessions_for(session2, ms::PromptSessionContainer::ParticipantType::application), ElementsAre(prompt_session1));
    EXPECT_THAT(list_prompt_sessions_for(session1, ms::PromptSessionContainer::ParticipantType::prompt_provider), ElementsAre(prompt_session1, prompt_session2));

    EXPECT_THAT(list_prompt_sessions_for(session2), ElementsAre(prompt_session1, prompt_session1, prompt_session3));
    EXPECT_THAT(list_prompt_sessions_for(session3), ElementsAre(prompt_session2));
    EXPECT_THAT(list_prompt_sessions_for(session4), ElementsAre(prompt_session2));
}

TEST_F(PromptSessionContainer, associates_processes_with_a_prompt_session_until_it_is_removed)
{
    container.insert_prompt_session(prompt_session1);

    container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session1.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session1.get(), session3, ms::PromptSessionContainer::ParticipantType::prompt_provider);

    EXPECT_THAT(count_participants_for(prompt_session1), Eq(3));

    container.remove_prompt_session(prompt_session1);

    EXPECT_THAT(count_participants_for(prompt_session1), Eq(0));
}

TEST_F(PromptSessionContainer, associates_prompt_sessions_with_a_participant_until_it_is_removed)
{
    container.insert_prompt_session(prompt_session1);
    container.insert_prompt_session(prompt_session2);

    container.insert_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session1.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider);

    container.insert_participant(prompt_session2.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider);
    container.insert_participant(prompt_session2.get(), session2, ms::PromptSessionContainer::ParticipantType::prompt_provider);

    EXPECT_THAT(list_participants_for(prompt_session1), ElementsAre(session1, session2));
    EXPECT_THAT(list_participants_for(prompt_session2), ElementsAre(session1, session2));

    container.remove_participant(prompt_session1.get(), session1, ms::PromptSessionContainer::ParticipantType::prompt_provider);

    EXPECT_THAT(list_participants_for(prompt_session1), ElementsAre(session2));
    EXPECT_THAT(list_participants_for(prompt_session2), ElementsAre(session1, session2));
}
