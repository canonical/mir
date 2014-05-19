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

#include "mir/frontend/event_sink.h"
#include "mir/scene/trust_session_creation_parameters.h"
#include "src/server/scene/trust_session_impl.h"
#include "src/server/scene/trust_session_container.h"
#include "src/server/scene/trust_session_participants.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_scene_session.h"
#include "mir_test_doubles/mock_trust_session_listener.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;


namespace
{

struct TrustSession : public testing::Test
{
    TrustSession()
    {
    }

    testing::NiceMock<mtd::MockTrustSessionListener> trust_session_listener;
    testing::NiceMock<mtd::MockSceneSession> trusted_helper;
    testing::NiceMock<mtd::MockSceneSession> trusted_app1;
    testing::NiceMock<mtd::MockSceneSession> trusted_app2;
    ms::TrustSessionContainer container;
};
}

TEST_F(TrustSession, start_and_stop)
{
    using namespace testing;

    auto shared_helper = mt::fake_shared(trusted_helper);
    std::shared_ptr<ms::Session> shared_app1 = mt::fake_shared(trusted_app1);
    std::shared_ptr<ms::Session> shared_app2 = mt::fake_shared(trusted_app2);

    auto trust_session = std::make_shared<ms::TrustSessionImpl>(shared_helper,
                               ms::a_trust_session(),
                               mt::fake_shared(trust_session_listener),
                               mt::fake_shared(container));

    EXPECT_CALL(trusted_helper, begin_trust_session()).Times(1);
    EXPECT_CALL(trusted_helper, end_trust_session()).Times(1);

    EXPECT_CALL(trusted_app1, begin_trust_session()).Times(0);
    EXPECT_CALL(trusted_app1, end_trust_session()).Times(0);

    EXPECT_CALL(trusted_app2, begin_trust_session()).Times(0);
    EXPECT_CALL(trusted_app2, end_trust_session()).Times(0);

    EXPECT_CALL(trust_session_listener, trusted_session_beginning(_, shared_app1)).Times(1);
    EXPECT_CALL(trust_session_listener, trusted_session_beginning(_, shared_app2)).Times(1);

    EXPECT_CALL(trust_session_listener, trusted_session_ending(_, shared_app1)).Times(1);
    EXPECT_CALL(trust_session_listener, trusted_session_ending(_, shared_app2)).Times(1);

    trust_session->start();
    trust_session->add_trusted_child(shared_app1);
    trust_session->add_trusted_child(shared_app2);
    trust_session->stop();
}

TEST_F(TrustSession, participant_memory)
{
    auto shared_helper = mt::fake_shared(trusted_helper);
    std::shared_ptr<ms::Session> shared_app1 = mt::fake_shared(trusted_app1);
    std::shared_ptr<ms::Session> shared_app2 = mt::fake_shared(trusted_app2);

    std::weak_ptr<ms::TrustSessionImpl> trust_session_weak;
    {
        auto trust_session = std::make_shared<ms::TrustSessionImpl>(shared_helper,
                                   ms::a_trust_session(),
                                   mt::fake_shared(trust_session_listener),
                                   mt::fake_shared(container));

        trust_session->start();
        trust_session->add_trusted_child(shared_app1);
        trust_session->add_trusted_child(shared_app2);
        trust_session->stop();

        trust_session_weak = trust_session;
    }

    // should have been deleted.
    EXPECT_TRUE(trust_session_weak.expired());
}
