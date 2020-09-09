/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "src/server/frontend_xwayland/xwayland_client_manager.h"
#include "mir/test/doubles/stub_shell.h"
#include "mir/test/doubles/stub_session.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace testing;

namespace
{
struct MockShell : mtd::StubShell
{
    MOCK_METHOD3(open_session, std::shared_ptr<ms::Session>(
        pid_t, std::string const&, std::shared_ptr<mf::EventSink> const&));
    MOCK_METHOD1(close_session, void(std::shared_ptr<ms::Session> const&));
};

struct XWaylandClientManagerTest : Test
{
    NiceMock<MockShell> shell;
    mtd::StubSession session_1;
    mtd::StubSession session_2;
    mtd::StubSession session_3;
    int owner_a; ///< Only used for it's address, value/type not important
    int owner_b; ///< Only used for it's address, value/type not important
    int owner_c; ///< Only used for it's address, value/type not important

    void SetUp() override
    {
        ON_CALL(shell, open_session(_, _, _))
            .WillByDefault(Return(mt::fake_shared(session_1)));
    }
};

}

TEST_F(XWaylandClientManagerTest, can_be_created)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};
}

TEST_F(XWaylandClientManagerTest, get_session_initially_creates_session)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(1, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));

    EXPECT_THAT(manager.register_owner_for_client(&owner_a, 1).get(), Eq(&session_1));
}

TEST_F(XWaylandClientManagerTest, repeated_get_session_with_same_pid_returns_same_session)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(1, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));

    EXPECT_THAT(manager.register_owner_for_client(&owner_a, 1).get(), Eq(&session_1));
    EXPECT_THAT(manager.register_owner_for_client(&owner_b, 1).get(), Eq(&session_1));
    EXPECT_THAT(manager.register_owner_for_client(&owner_c, 1).get(), Eq(&session_1));
}

TEST_F(XWaylandClientManagerTest, repeated_get_session_with_different_pids_creates_new_sessions)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(_, _, _))
        .Times(3)
        .WillOnce(Return(mt::fake_shared(session_1)))
        .WillOnce(Return(mt::fake_shared(session_2)))
        .WillOnce(Return(mt::fake_shared(session_3)));

    EXPECT_THAT(manager.register_owner_for_client(&owner_a, 1).get(), Eq(&session_1));
    EXPECT_THAT(manager.register_owner_for_client(&owner_b, 2).get(), Eq(&session_2));
    EXPECT_THAT(manager.register_owner_for_client(&owner_c, 3).get(), Eq(&session_3));
}

TEST_F(XWaylandClientManagerTest, release_closes_session_if_only_one_owner)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(_, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));
    EXPECT_CALL(shell, close_session(Eq(mt::fake_shared(session_1))))
        .Times(1);

    manager.register_owner_for_client(&owner_a, 1);
    manager.release(&owner_a);
}

TEST_F(XWaylandClientManagerTest, release_does_not_close_session_if_multiple_owners)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(_, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));
    EXPECT_CALL(shell, close_session(Eq(mt::fake_shared(session_1))))
        .Times(0);

    manager.register_owner_for_client(&owner_a, 1);
    manager.register_owner_for_client(&owner_b, 1);
    manager.release(&owner_a);

    Mock::VerifyAndClearExpectations(&shell);
}

TEST_F(XWaylandClientManagerTest, closes_all_active_sessions_on_destroy)
{
    EXPECT_CALL(shell, open_session(_, _, _))
        .Times(2)
        .WillOnce(Return(mt::fake_shared(session_1)))
        .WillOnce(Return(mt::fake_shared(session_2)));
    EXPECT_CALL(shell, close_session(Eq(mt::fake_shared(session_1))))
        .Times(1);
    EXPECT_CALL(shell, close_session(Eq(mt::fake_shared(session_2))))
        .Times(1);

    {
        mf::XWaylandClientManager manager{mt::fake_shared(shell)};
        manager.register_owner_for_client(&owner_a, 1);
        manager.register_owner_for_client(&owner_b, 1);
        manager.register_owner_for_client(&owner_c, 2);
    }
}

TEST_F(XWaylandClientManagerTest, session_closed_when_all_owners_release)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(_, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));
    EXPECT_CALL(shell, close_session(Eq(mt::fake_shared(session_1))))
        .Times(1);

    manager.register_owner_for_client(&owner_a, 1);
    manager.register_owner_for_client(&owner_b, 1);
    manager.register_owner_for_client(&owner_c, 1);
    manager.release(&owner_a);
    manager.release(&owner_b);
    manager.release(&owner_c);
}

TEST_F(XWaylandClientManagerTest, new_session_created_after_old_session_for_same_pid_closed)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};
    manager.register_owner_for_client(&owner_a, 1);
    manager.release(&owner_a);

    EXPECT_CALL(shell, open_session(_, _, _))
        .Times(1)
        .WillRepeatedly(Return(mt::fake_shared(session_2)));
    EXPECT_THAT(manager.register_owner_for_client(&owner_a, 1).get(), Eq(&session_2));
}

TEST_F(XWaylandClientManagerTest, get_session_with_active_owner_and_same_pid_throws)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};
    manager.register_owner_for_client(&owner_a, 1);
    EXPECT_THROW(manager.register_owner_for_client(&owner_a, 1), std::logic_error);
}

TEST_F(XWaylandClientManagerTest, get_session_with_active_owner_and_different_pid_throws)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};
    manager.register_owner_for_client(&owner_a, 1);
    EXPECT_THROW(manager.register_owner_for_client(&owner_a, 2), std::logic_error);
}
