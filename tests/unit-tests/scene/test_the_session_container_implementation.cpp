/*
 * Copyright © 2012-2019 Canonical Ltd.
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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#include "src/server/scene/application_session.h"
#include "mir/test/doubles/stub_session.h"
#include "mir/scene/session_container.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mtd = mir::test::doubles;

TEST(SessionContainer, for_each)
{
    using namespace ::testing;
    ms::SessionContainer container;

    auto session1 = std::make_shared<mtd::StubSession>();
    auto session2 = std::make_shared<mtd::StubSession>();

    container.insert_session(session1);
    container.insert_session(session2);

    struct local
    {
        MOCK_METHOD1(see, void(std::shared_ptr<ms::Session> const&));
        void operator()(std::shared_ptr<ms::Session> const& session)
        {
            see(session);
        }
    } functor;

    InSequence seq;
    EXPECT_CALL(functor, see(Eq(session1)));
    EXPECT_CALL(functor, see(Eq(session2)));

    container.for_each(std::ref(functor));
}

TEST(SessionContainer, successor_of)
{
    using namespace ::testing;
    ms::SessionContainer container;

    auto session1 = std::make_shared<mtd::StubSession>();
    auto session2 = std::make_shared<mtd::StubSession>();
    auto session3 = std::make_shared<mtd::StubSession>();

    container.insert_session(session1);
    container.insert_session(session2);
    container.insert_session(session3);

    EXPECT_EQ(session2, container.successor_of(session1));
    EXPECT_EQ(session3, container.successor_of(session2));
    EXPECT_EQ(session1, container.successor_of(session3));

    // Successor of no session is the last session.
    EXPECT_EQ(session3, container.successor_of(std::shared_ptr<ms::Session>()));
}

TEST(SessionContainer, invalid_session_throw_behavior)
{
    using namespace ::testing;
    ms::SessionContainer container;

    EXPECT_THROW({
        container.remove_session(std::make_shared<mtd::StubSession>());
    }, std::logic_error);
}
