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
#include "src/server/scene/trust_session_participants.h"
#include "mir_test_doubles/mock_scene_session.h"
#include "mir_test_doubles/null_trust_session.h"
#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(TrustSessionContainer, test_insert_participant_non_existing)
{
   using namespace testing;
    typedef testing::NiceMock<mtd::MockSceneSession> NiceMockSceneSession;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;

    auto session1 = std::make_shared<NiceMockSceneSession>();

    ms::TrustSessionContainer container;
    EXPECT_EQ(container.insert_participant(&trust_session1, session1), false);
}

TEST(TrustSessionContainer, test_insert_participant)
{
    using namespace testing;
    typedef testing::NiceMock<mtd::MockSceneSession> NiceMockSceneSession;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;
    testing::NiceMock<mtd::NullTrustSession> trust_session3;

    auto session1 = std::make_shared<NiceMockSceneSession>();
    auto session2 = std::make_shared<NiceMockSceneSession>();
    auto session3 = std::make_shared<NiceMockSceneSession>();
    auto session4 = std::make_shared<NiceMockSceneSession>();

    ms::TrustSessionContainer container;
    container.insert_trust_session(mt::fake_shared(trust_session1));
    container.insert_trust_session(mt::fake_shared(trust_session2));
    container.insert_trust_session(mt::fake_shared(trust_session3));

    EXPECT_EQ(container.insert_participant(&trust_session1, session1), true);
    EXPECT_EQ(container.insert_participant(&trust_session1, session2), true);
    EXPECT_EQ(container.insert_participant(&trust_session1, session3), true);
    EXPECT_EQ(container.insert_participant(&trust_session1, session1), false);

    EXPECT_EQ(container.insert_participant(&trust_session2, session2), true);
    EXPECT_EQ(container.insert_participant(&trust_session2, session3), true);
    EXPECT_EQ(container.insert_participant(&trust_session2, session2), false);

    EXPECT_EQ(container.insert_participant(&trust_session3, session3), true);
    EXPECT_EQ(container.insert_participant(&trust_session3, session3), false);
}

TEST(TrustSessionContainer, for_each_participant_for_trust_session)
{
    using namespace testing;
    typedef testing::NiceMock<mtd::MockSceneSession> NiceMockSceneSession;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;
    testing::NiceMock<mtd::NullTrustSession> trust_session3;

    auto session1 = std::make_shared<NiceMockSceneSession>();
    auto session2 = std::make_shared<NiceMockSceneSession>();
    auto session3 = std::make_shared<NiceMockSceneSession>();
    auto session4 = std::make_shared<NiceMockSceneSession>();

    ms::TrustSessionContainer container;
    container.insert_trust_session(mt::fake_shared(trust_session1));
    container.insert_trust_session(mt::fake_shared(trust_session2));
    container.insert_trust_session(mt::fake_shared(trust_session3));

    container.insert_participant(&trust_session1, session1);
    container.insert_participant(&trust_session1, session2);
    container.insert_participant(&trust_session2, session3);
    container.insert_participant(&trust_session2, session1);
    container.insert_participant(&trust_session3, session2);
    container.insert_participant(&trust_session2, session4);

    std::vector<std::shared_ptr<mf::Session>> results;
    auto for_each_fn = [&results](std::weak_ptr<mf::Session> const& session)
    {
        results.push_back(session.lock());
    };

    container.for_each_participant_for_trust_session(&trust_session1, for_each_fn);
    EXPECT_THAT(results, ElementsAre(session1, session2));

    results.clear();
    container.for_each_participant_for_trust_session(&trust_session2, for_each_fn);
    EXPECT_THAT(results, ElementsAre(session3, session1, session4));

    results.clear();
    container.for_each_participant_for_trust_session(&trust_session3, for_each_fn);
    EXPECT_THAT(results, ElementsAre(session2));
}

TEST(TrustSessionContainer, for_each_trust_session_for_participant)
{
    using namespace testing;
    typedef testing::NiceMock<mtd::MockSceneSession> NiceMockSceneSession;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;
    testing::NiceMock<mtd::NullTrustSession> trust_session3;

    auto session1 = std::make_shared<NiceMockSceneSession>();
    auto session2 = std::make_shared<NiceMockSceneSession>();
    auto session3 = std::make_shared<NiceMockSceneSession>();
    auto session4 = std::make_shared<NiceMockSceneSession>();

    ms::TrustSessionContainer container;
    container.insert_trust_session(mt::fake_shared(trust_session1));
    container.insert_trust_session(mt::fake_shared(trust_session2));
    container.insert_trust_session(mt::fake_shared(trust_session3));

    container.insert_participant(&trust_session1, session1);
    container.insert_participant(&trust_session1, session2);
    container.insert_participant(&trust_session2, session3);
    container.insert_participant(&trust_session2, session1);
    container.insert_participant(&trust_session3, session2);
    container.insert_participant(&trust_session2, session4);

    std::vector<std::shared_ptr<mf::TrustSession>> results;
    auto for_each_fn = [&results](std::shared_ptr<mf::TrustSession> const& trust_session)
    {
        results.push_back(trust_session);
    };

    container.for_each_trust_session_for_participant(session2, for_each_fn);
    EXPECT_THAT(results, ElementsAre(mt::fake_shared(trust_session1), mt::fake_shared(trust_session3)));
}

TEST(TrustSessionContainer, test_remove_trust_sesion)
{
    using namespace testing;
    typedef testing::NiceMock<mtd::MockSceneSession> NiceMockSceneSession;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;

    auto session1 = std::make_shared<NiceMockSceneSession>();
    auto session2 = std::make_shared<NiceMockSceneSession>();
    auto session3 = std::make_shared<NiceMockSceneSession>();

    ms::TrustSessionContainer container;
    container.insert_trust_session(mt::fake_shared(trust_session1));

    container.insert_participant(&trust_session1, session1);
    container.insert_participant(&trust_session1, session2);
    container.insert_participant(&trust_session1, session3);

    std::vector<std::shared_ptr<mf::Session>> results;
    auto for_each_fn = [&results](std::weak_ptr<mf::Session> const& session)
    {
        results.push_back(session.lock());
    };

    container.for_each_participant_for_trust_session(&trust_session1, for_each_fn);
    EXPECT_THAT(results, ElementsAre(session1, session2, session3));

    container.remove_trust_session(mt::fake_shared(trust_session1));

    results.clear();
    container.for_each_participant_for_trust_session(&trust_session1, for_each_fn);
    EXPECT_THAT(results, ElementsAre());
}

TEST(TrustSessionContainer, test_remove_partcicipant)
{
    using namespace testing;
    typedef testing::NiceMock<mtd::MockSceneSession> NiceMockSceneSession;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;

    auto session1 = std::make_shared<NiceMockSceneSession>();
    auto session2 = std::make_shared<NiceMockSceneSession>();
    auto session3 = std::make_shared<NiceMockSceneSession>();

    ms::TrustSessionContainer container;
    container.insert_trust_session(mt::fake_shared(trust_session1));
    container.insert_trust_session(mt::fake_shared(trust_session2));

    container.insert_participant(&trust_session1, session1);
    container.insert_participant(&trust_session1, session2);
    container.insert_participant(&trust_session1, session3);

    container.insert_participant(&trust_session2, session1);
    container.insert_participant(&trust_session2, session2);
    container.insert_participant(&trust_session2, session3);

    std::vector<std::shared_ptr<mf::Session>> results;
    auto for_each_fn = [&results](std::weak_ptr<mf::Session> const& session)
    {
        results.push_back(session.lock());
    };

    container.remove_participant(&trust_session1, session1);
    container.remove_participant(&trust_session1, session2);

    container.for_each_participant_for_trust_session(&trust_session1, for_each_fn);
    EXPECT_THAT(results, ElementsAre(session3));

    results.clear();
    container.for_each_participant_for_trust_session(&trust_session2, for_each_fn);
    EXPECT_THAT(results, ElementsAre(session1, session2, session3));
}

TEST(TrustSessionContainer, waiting_process_removed_on_insert_matching_session)
{
    using namespace testing;
    typedef testing::NiceMock<mtd::MockSceneSession> NiceMockSceneSession;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;

    auto session1 = std::make_shared<NiceMockSceneSession>();
    auto session2 = std::make_shared<NiceMockSceneSession>();
    auto session3 = std::make_shared<NiceMockSceneSession>();

    ON_CALL(*session1, process_id()).WillByDefault(Return(1));
    ON_CALL(*session2, process_id()).WillByDefault(Return(1));
    ON_CALL(*session3, process_id()).WillByDefault(Return(2));

    ms::TrustSessionContainer container;
    container.insert_trust_session(mt::fake_shared(trust_session1));
    container.insert_trust_session(mt::fake_shared(trust_session2));

    container.insert_waiting_process(&trust_session1, 1);
    container.insert_waiting_process(&trust_session1, 1);
    container.insert_waiting_process(&trust_session2, 1);

    std::vector<std::shared_ptr<mf::TrustSession>> results;
    auto for_each_fn = [&results](std::shared_ptr<mf::TrustSession> const& trust_session)
    {
        results.push_back(trust_session);
    };

    // At this point, we're waiting for:
    // trust session 1 -> 2 process with pid = 1
    // trust session 2 -> 1 process with pid = 1

    container.for_each_trust_session_for_waiting_process(1, for_each_fn);
    EXPECT_THAT(results, ElementsAre(mt::fake_shared(trust_session1), mt::fake_shared(trust_session1), mt::fake_shared(trust_session2)));

    container.insert_participant(&trust_session1, session3);
    // inserted session3 into trust_session1 (pid=2) - wasn't waiting for it. no change.

    results.clear();
    container.for_each_trust_session_for_waiting_process(1, for_each_fn);
    EXPECT_THAT(results, ElementsAre(mt::fake_shared(trust_session1), mt::fake_shared(trust_session1), mt::fake_shared(trust_session2)));

    container.insert_participant(&trust_session1, session1);
    // inserted session1 into trust_session1 (pid=1) - was waiting for it. should be 1 left for trust_session1 & 1 for trust_session2

    results.clear();
    container.for_each_trust_session_for_waiting_process(1, for_each_fn);
    EXPECT_THAT(results, ElementsAre(mt::fake_shared(trust_session1), mt::fake_shared(trust_session2)));

    container.insert_participant(&trust_session2, session1);
    // inserted session2 into trust_session2 (pid=1) - was waiting for it. should be 1 left for trust_session1

    results.clear();
    container.for_each_trust_session_for_waiting_process(1, for_each_fn);
    EXPECT_THAT(results, ElementsAre(mt::fake_shared(trust_session1)));

    container.insert_participant(&trust_session1, session2);
    // inserted session2 into trust_session1 (pid=1) - was waiting for it. none left

    results.clear();
    container.for_each_trust_session_for_waiting_process(1, for_each_fn);
    EXPECT_THAT(results, ElementsAre());
}

TEST(TrustSessionParticipants, test_insert)
{
    using namespace testing;
    typedef testing::NiceMock<mtd::MockSceneSession> NiceMockSceneSession;
    typedef testing::NiceMock<mtd::NullTrustSession> NiceMockTrustSession;

    NiceMockTrustSession trust_session1;
    NiceMockTrustSession trust_session2;

    auto session1 = std::make_shared<NiceMockSceneSession>();
    auto session2 = std::make_shared<NiceMockSceneSession>();
    auto session3 = std::make_shared<NiceMockSceneSession>();

    ms::TrustSessionContainer container;
    container.insert_trust_session(mt::fake_shared(trust_session1));
    container.insert_trust_session(mt::fake_shared(trust_session2));

    container.insert_participant(&trust_session1, session1);
    container.insert_participant(&trust_session1, session2);
    container.insert_participant(&trust_session1, session3);
    container.insert_participant(&trust_session2, session2);

    ms::TrustSessionParticipants trust_session1_participants(&trust_session1, mt::fake_shared(container));
    ms::TrustSessionParticipants trust_session2_participants(&trust_session2, mt::fake_shared(container));

    std::vector<std::shared_ptr<mf::Session>> results;
    auto for_each_fn = [&](std::weak_ptr<mf::Session> const& session)
    {
        results.push_back(session.lock());
    };

    trust_session1_participants.for_each_participant(for_each_fn);
    EXPECT_THAT(results, ElementsAre(session1, session3));

    results.clear();
    trust_session2_participants.for_each_participant(for_each_fn);
    EXPECT_THAT(results, ElementsAre(session2));
}
