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

#include "src/server/scene/trust_session_impl.h"
#include "src/server/scene/trust_session_container.h"
#include "mir/scene/trust_session_creation_parameters.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_scene_session.h"
#include "mir_test_doubles/mock_trust_session_listener.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
struct TrustSession : public testing::Test
{
    testing::NiceMock<mtd::MockTrustSessionListener> trust_session_listener;
    ms::TrustSessionContainer container;
    mtd::StubSceneSession trusted_helper;
    mtd::StubSceneSession trusted_app1;
    mtd::StubSceneSession trusted_app2;

    std::shared_ptr<ms::Session> shared_helper = mt::fake_shared(trusted_helper);
    std::shared_ptr<ms::Session> shared_app1 = mt::fake_shared(trusted_app1);
    std::shared_ptr<ms::Session> shared_app2 = mt::fake_shared(trusted_app2);

    ms::TrustSessionImpl trust_session{shared_helper,
                                   ms::a_trust_session(),
                                   mt::fake_shared(trust_session_listener),
                                   mt::fake_shared(container)};

    void SetUp()
    {
        container.insert_trust_session(mt::fake_shared(trust_session));
    }

    void TearDown()
    {
        trust_session.stop();
    }
};
}

TEST_F(TrustSession, trusted_child_apps_get_start_and_stop_notifications)
{
    EXPECT_CALL(trust_session_listener, trusted_session_beginning(_, shared_app1)).Times(1);
    EXPECT_CALL(trust_session_listener, trusted_session_beginning(_, shared_app2)).Times(1);

    EXPECT_CALL(trust_session_listener, trusted_session_ending(_, shared_app1)).Times(1);
    EXPECT_CALL(trust_session_listener, trusted_session_ending(_, shared_app2)).Times(1);

    trust_session.add_trusted_participant(shared_app1);
    trust_session.add_trusted_participant(shared_app2);
}
