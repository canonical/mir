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

#include "src/server/scene/trust_session_container.h"
#include "mir_test_doubles/mock_scene_session.h"
#include "mir_test_doubles/null_trust_session.h"
#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
struct TrustSessionContainer : testing::Test
{
    static constexpr pid_t process1 = 1;
    static constexpr pid_t process2 = 2;

    mtd::NullTrustSession null_trust_session1;
    mtd::NullTrustSession null_trust_session2;
    mtd::NullTrustSession null_trust_session3;

    std::shared_ptr<ms::TrustSession> const trust_session1 = mt::fake_shared(null_trust_session1);
    std::shared_ptr<ms::TrustSession> const trust_session2 = mt::fake_shared(null_trust_session2);
    std::shared_ptr<ms::TrustSession> const trust_session3 = mt::fake_shared(null_trust_session3);

    testing::NiceMock<mtd::MockSceneSession> mock_scene_session1;
    testing::NiceMock<mtd::MockSceneSession> mock_scene_session2;
    testing::NiceMock<mtd::MockSceneSession> mock_scene_session3;
    testing::NiceMock<mtd::MockSceneSession> mock_scene_session4;

    std::shared_ptr<ms::Session> const session1 = mt::fake_shared(mock_scene_session1);
    std::shared_ptr<ms::Session> const session2 = mt::fake_shared(mock_scene_session2);
    std::shared_ptr<ms::Session> const session3 = mt::fake_shared(mock_scene_session3);
    std::shared_ptr<ms::Session> const session4 = mt::fake_shared(mock_scene_session4);

    ms::TrustSessionContainer container;

    void SetUp() {
        ON_CALL(mock_scene_session1, process_id()).WillByDefault(Return(process1));
        ON_CALL(mock_scene_session2, process_id()).WillByDefault(Return(process1));
        ON_CALL(mock_scene_session3, process_id()).WillByDefault(Return(process2));
        ON_CALL(mock_scene_session4, process_id()).WillByDefault(Return(process2));
    }

    std::vector<std::shared_ptr<ms::Session>> list_participants_for(std::shared_ptr<ms::TrustSession> const& trust_session)
    {
        std::vector<std::shared_ptr<ms::Session>> results;
        auto list_participants = [&results](std::weak_ptr<ms::Session> const& session, ms::TrustSessionContainer::TrustType)
        {
            results.push_back(session.lock());
        };

        container.for_each_participant_in_trust_session(trust_session.get(), list_participants);

        return results;
    }

    int count_participants_for(std::shared_ptr<ms::TrustSession> const& trust_session)
    {
        return list_participants_for(trust_session).size();
    }

    std::vector<std::shared_ptr<ms::TrustSession>> list_trust_sessions_for(std::shared_ptr<ms::Session> const& session)
    {
        std::vector<std::shared_ptr<ms::TrustSession>> results;
        auto list_trust_sessions = [&results](std::shared_ptr<ms::TrustSession> const& trust_session)
        {
            results.push_back(trust_session);
        };

        container.for_each_trust_session_with_participant(session, list_trust_sessions);

        return results;
    }

    std::vector<std::shared_ptr<ms::TrustSession>> list_trust_sessions_for(std::shared_ptr<ms::Session> const& session, ms::TrustSessionContainer::TrustType trust_type)
    {
        std::vector<std::shared_ptr<ms::TrustSession>> results;
        auto list_trust_sessions = [&results](std::shared_ptr<ms::TrustSession> const& trust_session)
        {
            results.push_back(trust_session);
        };

        container.for_each_trust_session_with_participant(session, trust_type, list_trust_sessions);

        return results;
    }

    int count_trust_sessions_for(std::shared_ptr<ms::Session> const& session)
    {
        return list_trust_sessions_for(session).size();
    }

    std::vector<std::shared_ptr<ms::TrustSession>> list_trust_sessions_for(pid_t process)
    {
        std::vector<std::shared_ptr<ms::TrustSession>> results;
        auto list_trust_sessions = [&results](std::shared_ptr<ms::TrustSession> const& trust_session)
        {
            results.push_back(trust_session);
        };

        container.for_each_trust_session_expecting_process(process, list_trust_sessions);

        return results;
    }
};
}

TEST_F(TrustSessionContainer, throws_if_no_trust_session)
{
    EXPECT_THROW(
        container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session),
        std::runtime_error);
}

TEST_F(TrustSessionContainer, insert_true_if_trust_session_exists)
{
    container.insert_trust_session(trust_session1);
    EXPECT_TRUE(container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session));
}

TEST_F(TrustSessionContainer, insert_true_if_not_duplicate)
{
    container.insert_trust_session(trust_session1);
    container.insert_trust_session(trust_session2);
    container.insert_trust_session(trust_session3);

    EXPECT_TRUE(container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session));
    EXPECT_TRUE(container.insert_participant(trust_session1.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session));
    EXPECT_TRUE(container.insert_participant(trust_session1.get(), session3, ms::TrustSessionContainer::TrustType::trusted_session));
    EXPECT_FALSE(container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session));
    EXPECT_TRUE(container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::helper_session));

    EXPECT_TRUE(container.insert_participant(trust_session2.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session));
    EXPECT_TRUE(container.insert_participant(trust_session2.get(), session3, ms::TrustSessionContainer::TrustType::trusted_session));
    EXPECT_FALSE(container.insert_participant(trust_session2.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session));

    EXPECT_TRUE(container.insert_participant(trust_session3.get(), session3, ms::TrustSessionContainer::TrustType::trusted_session));
    EXPECT_FALSE(container.insert_participant(trust_session3.get(), session3, ms::TrustSessionContainer::TrustType::trusted_session));
}

TEST_F(TrustSessionContainer, lists_participants_in_a_trust_session)
{
    container.insert_trust_session(trust_session1);
    container.insert_trust_session(trust_session2);
    container.insert_trust_session(trust_session3);

    container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session1.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session2.get(), session3, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session2.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session3.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session2.get(), session4, ms::TrustSessionContainer::TrustType::trusted_session);

    EXPECT_THAT(list_participants_for(trust_session1), ElementsAre(session1, session2));
    EXPECT_THAT(list_participants_for(trust_session2), ElementsAre(session3, session1, session4));
    EXPECT_THAT(list_participants_for(trust_session3), ElementsAre(session2));
}

TEST_F(TrustSessionContainer, lists_trust_sessions_for_a_participant)
{
    container.insert_trust_session(trust_session1);
    container.insert_trust_session(trust_session2);
    container.insert_trust_session(trust_session3);

    container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::helper_session);
    container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session1.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session2.get(), session3, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session2.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session3.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session2.get(), session4, ms::TrustSessionContainer::TrustType::trusted_session);

    EXPECT_THAT(list_trust_sessions_for(session1), ElementsAre(trust_session1, trust_session1, trust_session2));
    EXPECT_THAT(list_trust_sessions_for(session1, ms::TrustSessionContainer::TrustType::helper_session), ElementsAre(trust_session1));
    EXPECT_THAT(list_trust_sessions_for(session1, ms::TrustSessionContainer::TrustType::trusted_session), ElementsAre(trust_session1, trust_session2));

    EXPECT_THAT(list_trust_sessions_for(session2), ElementsAre(trust_session1, trust_session3));
    EXPECT_THAT(list_trust_sessions_for(session3), ElementsAre(trust_session2));
    EXPECT_THAT(list_trust_sessions_for(session4), ElementsAre(trust_session2));
}

TEST_F(TrustSessionContainer, associates_processes_with_a_trust_session_until_it_is_removed)
{
    container.insert_trust_session(trust_session1);

    container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session1.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session1.get(), session3, ms::TrustSessionContainer::TrustType::trusted_session);

    EXPECT_THAT(count_participants_for(trust_session1), Eq(3));

    container.remove_trust_session(trust_session1);

    EXPECT_THAT(count_participants_for(trust_session1), Eq(0));
}

TEST_F(TrustSessionContainer, associates_trust_sessions_with_a_participant_until_it_is_removed)
{
    container.insert_trust_session(trust_session1);
    container.insert_trust_session(trust_session2);

    container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session1.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session);

    container.insert_participant(trust_session2.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    container.insert_participant(trust_session2.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session);

    EXPECT_THAT(list_participants_for(trust_session1), ElementsAre(session1, session2));
    EXPECT_THAT(list_participants_for(trust_session2), ElementsAre(session1, session2));

    container.remove_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);

    EXPECT_THAT(list_participants_for(trust_session1), ElementsAre(session2));
    EXPECT_THAT(list_participants_for(trust_session2), ElementsAre(session1, session2));
}

TEST_F(TrustSessionContainer, waiting_process_removed_on_insert_matching_session)
{
    container.insert_trust_session(trust_session1);
    container.insert_trust_session(trust_session2);

    container.insert_waiting_process(trust_session1.get(), process1);
    container.insert_waiting_process(trust_session1.get(), process1);
    container.insert_waiting_process(trust_session2.get(), process1);
    // At this point, we're waiting for:
    // trust session 1 -> 2 process with pid = 1
    // trust session 2 -> 1 process with pid = 1

    EXPECT_THAT(list_trust_sessions_for(process1), ElementsAre(trust_session1, trust_session1, trust_session2));

    container.insert_participant(trust_session1.get(), session3, ms::TrustSessionContainer::TrustType::trusted_session);
    // inserted session3 into trust_session1 (pid=2) - wasn't waiting for it. no change.
    EXPECT_THAT(list_trust_sessions_for(process1), ElementsAre(trust_session1, trust_session1, trust_session2));

    container.insert_participant(trust_session1.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    // inserted session1 into trust_session1 (pid=1) - was waiting for it. should be 1 left for trust_session1 & 1 for trust_session2
    EXPECT_THAT(list_trust_sessions_for(process1), ElementsAre(trust_session1, trust_session2));

    container.insert_participant(trust_session2.get(), session1, ms::TrustSessionContainer::TrustType::trusted_session);
    // inserted session2 into trust_session2 (pid=1) - was waiting for it. should be 1 left for trust_session1
    EXPECT_THAT(list_trust_sessions_for(process1), ElementsAre(trust_session1));

    container.insert_participant(trust_session1.get(), session2, ms::TrustSessionContainer::TrustType::trusted_session);
    // inserted session2 into trust_session1 (pid=1) - was waiting for it. none left
    EXPECT_THAT(list_trust_sessions_for(process1), ElementsAre());
}
