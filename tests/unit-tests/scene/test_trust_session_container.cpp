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


TEST(TrustSessionContainer, test_insert)
{
    using namespace testing;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;
    testing::NiceMock<mtd::NullTrustSession> trust_session3;

    ms::TrustSessionContainer container;
    EXPECT_EQ(container.insert(mt::fake_shared(trust_session1), 1), true);
    EXPECT_EQ(container.insert(mt::fake_shared(trust_session1), 2), true);
    EXPECT_EQ(container.insert(mt::fake_shared(trust_session1), 3), true);
    EXPECT_EQ(container.insert(mt::fake_shared(trust_session1), 1), false);

    EXPECT_EQ(container.insert(mt::fake_shared(trust_session2), 2), true);
    EXPECT_EQ(container.insert(mt::fake_shared(trust_session2), 3), true);
    EXPECT_EQ(container.insert(mt::fake_shared(trust_session2), 2), false);

    EXPECT_EQ(container.insert(mt::fake_shared(trust_session3), 3), true);
    EXPECT_EQ(container.insert(mt::fake_shared(trust_session3), 3), false);
}

TEST(TrustSessionContainer, for_each_process_for_trust_session)
{
    using namespace testing;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;
    testing::NiceMock<mtd::NullTrustSession> trust_session3;

    ms::TrustSessionContainer container;
    container.insert(mt::fake_shared(trust_session1), 1);
    container.insert(mt::fake_shared(trust_session1), 2);
    container.insert(mt::fake_shared(trust_session2), 3);
    container.insert(mt::fake_shared(trust_session2), 1);
    container.insert(mt::fake_shared(trust_session3), 2);
    container.insert(mt::fake_shared(trust_session2), 4);

    std::vector<ms::TrustSessionContainer::ClientProcess> results;
    auto for_each_fn = [&results](ms::TrustSessionContainer::ClientProcess const& process)
    {
        results.push_back(process);
    };

    container.for_each_process_for_trust_session(mt::fake_shared(trust_session2), for_each_fn);
    EXPECT_THAT(results, ElementsAre(3, 1, 4));
}

TEST(TrustSessionContainer, for_each_trust_session_for_process)
{
    using namespace testing;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;
    testing::NiceMock<mtd::NullTrustSession> trust_session3;

    ms::TrustSessionContainer container;
    container.insert(mt::fake_shared(trust_session1), 1);
    container.insert(mt::fake_shared(trust_session1), 2);
    container.insert(mt::fake_shared(trust_session2), 3);
    container.insert(mt::fake_shared(trust_session2), 1);
    container.insert(mt::fake_shared(trust_session3), 2);
    container.insert(mt::fake_shared(trust_session2), 4);

    std::vector<std::shared_ptr<mf::TrustSession>> results;
    auto for_each_fn = [&results](std::shared_ptr<mf::TrustSession> const& trust_session)
    {
        results.push_back(trust_session);
    };

    container.for_each_trust_session_for_process(2, for_each_fn);
    EXPECT_THAT(results, ElementsAre(mt::fake_shared(trust_session1), mt::fake_shared(trust_session3)));
}

TEST(TrustSessionContainer, test_remove_trust_sesion)
{
    using namespace testing;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;

    ms::TrustSessionContainer container;
    container.insert(mt::fake_shared(trust_session1), 1);
    container.insert(mt::fake_shared(trust_session1), 2);
    container.insert(mt::fake_shared(trust_session1), 3);

    int count = 0;
    auto for_each_fn = [&count](ms::TrustSessionContainer::ClientProcess const&)
    {
        count++;
    };

    container.for_each_process_for_trust_session(mt::fake_shared(trust_session1), for_each_fn);
    EXPECT_EQ(count, 3);

    container.remove_trust_session(mt::fake_shared(trust_session1));

    count = 0;
    container.for_each_process_for_trust_session(mt::fake_shared(trust_session1), for_each_fn);
    EXPECT_EQ(count, 0);
}

TEST(TrustSessionContainer, remove_process)
{
    using namespace testing;

    testing::NiceMock<mtd::NullTrustSession> trust_session1;
    testing::NiceMock<mtd::NullTrustSession> trust_session2;
    testing::NiceMock<mtd::NullTrustSession> trust_session3;

    ms::TrustSessionContainer container;
    container.insert(mt::fake_shared(trust_session1), 1);
    container.insert(mt::fake_shared(trust_session1), 2);

    container.insert(mt::fake_shared(trust_session2), 1);
    container.insert(mt::fake_shared(trust_session2), 3);

    container.insert(mt::fake_shared(trust_session3), 1);
    container.insert(mt::fake_shared(trust_session3), 4);

    int count = 0;
    auto for_each_fn = [&count](std::shared_ptr<mf::TrustSession> const&)
    {
        count++;
    };

    container.for_each_trust_session_for_process(1, for_each_fn);
    EXPECT_EQ(count, 3);

    container.remove_process(1);

    count = 0;
    container.for_each_trust_session_for_process(1, for_each_fn);
    EXPECT_EQ(count, 0);
}

