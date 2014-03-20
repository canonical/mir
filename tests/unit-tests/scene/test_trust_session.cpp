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
#include "mir/shell/trust_session_creation_parameters.h"
#include "src/server/scene/trust_session.h"
#include "src/server/scene/default_session_container.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test_doubles/mock_shell_session.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;


namespace
{

class MockEventSink : public mf::EventSink
{
public:
    MOCK_METHOD1(handle_event, void(MirEvent const&));
    MOCK_METHOD1(handle_lifecycle_event, void(MirLifecycleState));
    MOCK_METHOD1(handle_display_config_change, void(mir::graphics::DisplayConfiguration const&));
};
struct MockSessionContainer : public ms::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<msh::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<msh::Session> const&));
    MOCK_CONST_METHOD1(successor_of, std::shared_ptr<msh::Session>(std::shared_ptr<msh::Session> const&));
    MOCK_CONST_METHOD1(for_each, void(std::function<void(std::shared_ptr<msh::Session> const&)>));
    ~MockSessionContainer() noexcept {}
};

struct TrustSession : public testing::Test
{
    TrustSession()
    {
        container.insert_session(mt::fake_shared(trusted_helper));
        container.insert_session(mt::fake_shared(trusted_app1));
        container.insert_session(mt::fake_shared(trusted_app2));

        ON_CALL(trusted_helper, process_id()).WillByDefault(testing::Return(__LINE__));
        ON_CALL(trusted_app1, process_id()).WillByDefault(testing::Return(__LINE__));
        ON_CALL(trusted_app2, process_id()).WillByDefault(testing::Return(__LINE__));
    }

    ms::DefaultSessionContainer container;
    testing::NiceMock<MockSessionContainer> child_container;
    testing::NiceMock<MockEventSink> sender;

    testing::NiceMock<mtd::MockShellSession> trusted_helper;
    testing::NiceMock<mtd::MockShellSession> trusted_app1;
    testing::NiceMock<mtd::MockShellSession> trusted_app2;
};
}

TEST_F(TrustSession, start_and_stop)
{
    using namespace testing;

    auto helper = mt::fake_shared(trusted_helper);
    auto app1 = mt::fake_shared(trusted_app1);
    auto app2 = mt::fake_shared(trusted_app2);

    EXPECT_CALL(trusted_helper, begin_trust_session(_, ElementsAre(app1, app2))).Times(1);
    EXPECT_CALL(trusted_helper, end_trust_session()).Times(1);

    msh::TrustSessionCreationParameters parameters;
    auto trust_session = ms::TrustSession::start_for(helper,
                                                      parameters.add_application(trusted_app1.process_id())
                                                                .add_application(trusted_app2.process_id()),
                                                      mt::fake_shared(container));

    trust_session->stop();
}

TEST_F(TrustSession, multi_trust_sessions)
{
    using namespace testing;

    msh::TrustSessionCreationParameters parameters;
    auto trust_session1 = ms::TrustSession::start_for(mt::fake_shared(trusted_helper),
                                                      parameters,
                                                      mt::fake_shared(container));

    auto trust_session2 = ms::TrustSession::start_for(mt::fake_shared(trusted_helper),
                                                      parameters,
                                                      mt::fake_shared(container));

    auto trust_session3 = ms::TrustSession::start_for(mt::fake_shared(trusted_helper),
                                                      parameters,
                                                      mt::fake_shared(container));

    EXPECT_THAT(trust_session1, Ne(std::shared_ptr<mf::TrustSession>()));
    EXPECT_THAT(trust_session2, Ne(std::shared_ptr<mf::TrustSession>()));
    EXPECT_THAT(trust_session3, Ne(std::shared_ptr<mf::TrustSession>()));
}

