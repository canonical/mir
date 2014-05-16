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
    static constexpr pid_t process3 = 3;
    static constexpr pid_t process4 = 4;

    mtd::NullTrustSession null_trust_session1;
    mtd::NullTrustSession null_trust_session2;
    mtd::NullTrustSession null_trust_session3;

    std::shared_ptr<ms::TrustSession> const trust_session1 = mt::fake_shared(null_trust_session1);
    std::shared_ptr<ms::TrustSession> const trust_session2 = mt::fake_shared(null_trust_session2);
    std::shared_ptr<ms::TrustSession> const trust_session3 = mt::fake_shared(null_trust_session3);

    ms::TrustSessionContainer container;

    std::vector<ms::TrustSessionContainer::ClientProcess> list_processes_for(std::shared_ptr<ms::TrustSession> const& trust_session)
    {
        std::vector<ms::TrustSessionContainer::ClientProcess> results;
        auto list_processes = [&results](ms::TrustSessionContainer::ClientProcess const& process)
        {
            results.push_back(process);
        };

        container.for_each_process_for_trust_session(trust_session, list_processes);

        return results;
    }

    std::vector<std::shared_ptr<mf::TrustSession>> list_trust_sessions_for(pid_t process)
    {
        std::vector<std::shared_ptr<mf::TrustSession>> results;
        auto list_trust_sessions = [&results](std::shared_ptr<mf::TrustSession> const& trust_session)
        {
            results.push_back(trust_session);
        };

        container.for_each_trust_session_for_process(process, list_trust_sessions);

        return results;
    }

    int count_processes_for(std::shared_ptr<ms::TrustSession> const& trust_session)
    {
        int count = 0;
        auto count_processes = [&count](ms::TrustSessionContainer::ClientProcess const&)
        {
            count++;
        };

        container.for_each_process_for_trust_session(trust_session, count_processes);

        return count;
    }

    int count_trust_sessions_for(pid_t process)
    {
        int count = 0;
        auto count_trust_sessions = [&count](std::shared_ptr<mf::TrustSession> const&)
        {
            count++;
        };

        container.for_each_trust_session_for_process(process, count_trust_sessions);

        return count;
    }
};

// TODO only needed because ms::TrustSessionContainer::insert() wants a "pid_t const&" not a value
constexpr pid_t TrustSessionContainer::process1;
constexpr pid_t TrustSessionContainer::process2;
constexpr pid_t TrustSessionContainer::process3;
constexpr pid_t TrustSessionContainer::process4;
}

TEST_F(TrustSessionContainer, insert_true_iff_not_duplicate)
{
    EXPECT_TRUE(container.insert(trust_session1, process1));
    EXPECT_TRUE(container.insert(trust_session1, process2));
    EXPECT_TRUE(container.insert(trust_session1, process3));
    EXPECT_FALSE(container.insert(trust_session1, process1));

    EXPECT_TRUE(container.insert(trust_session2, process2));
    EXPECT_TRUE(container.insert(trust_session2, process3));
    EXPECT_FALSE(container.insert(trust_session2, process2));

    EXPECT_TRUE(container.insert(trust_session3, process3));
    EXPECT_FALSE(container.insert(trust_session3, process3));
}

TEST_F(TrustSessionContainer, lists_processes_in_a_trust_session)
{
    container.insert(trust_session1, process1);
    container.insert(trust_session1, process2);
    container.insert(trust_session2, process3);
    container.insert(trust_session2, process1);
    container.insert(trust_session3, process2);
    container.insert(trust_session2, process4);

    EXPECT_THAT(list_processes_for(trust_session2), ElementsAre(process3, process1, process4));
}

TEST_F(TrustSessionContainer, lists_trust_sessions_for_a_process)
{
    container.insert(trust_session1, process1);
    container.insert(trust_session1, process2);
    container.insert(trust_session2, process3);
    container.insert(trust_session2, process1);
    container.insert(trust_session3, process2);
    container.insert(trust_session2, process4);

    EXPECT_THAT(list_trust_sessions_for(process2), ElementsAre(trust_session1, trust_session3));
}

TEST_F(TrustSessionContainer, associates_processes_with_a_trust_session_until_it_is_removed)
{
    container.insert(trust_session1, process1);
    container.insert(trust_session1, process2);
    container.insert(trust_session1, process3);

    EXPECT_THAT(count_processes_for(trust_session1), Eq(3));

    container.remove_trust_session(trust_session1);

    EXPECT_THAT(count_processes_for(trust_session1), Eq(0));
}

TEST_F(TrustSessionContainer, associates_trust_sessions_with_a_process_until_it_is_removed)
{
    container.insert(trust_session1, process1);
    container.insert(trust_session1, process2);

    container.insert(trust_session2, process1);
    container.insert(trust_session2, process3);

    container.insert(trust_session3, process1);
    container.insert(trust_session3, process4);

    EXPECT_THAT(count_trust_sessions_for(process1), Eq(3));

    container.remove_process(process1);

    EXPECT_THAT(count_trust_sessions_for(process1), Eq(0));
}
