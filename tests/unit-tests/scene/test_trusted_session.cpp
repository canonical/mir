/*
 * Copyright © 2012-2014 Canonical Ltd.
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
#include "mir/shell/trusted_session_creation_parameters.h"
#include "src/server/scene/trusted_session.h"
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
    MOCK_METHOD2(handle_trusted_session_event, void(mir::frontend::SessionId session_id, MirTrustedSessionState state));
};
struct MockSessionContainer : public ms::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<msh::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<msh::Session> const&));
    MOCK_METHOD0(clear, void());
    MOCK_CONST_METHOD1(successor_of, std::shared_ptr<msh::Session>(std::shared_ptr<msh::Session> const&));
    MOCK_CONST_METHOD1(for_each, void(std::function<void(std::shared_ptr<msh::Session> const&)>));
    MOCK_CONST_METHOD2(for_each, void(std::function<bool(std::shared_ptr<msh::Session> const&)>, bool));
    ~MockSessionContainer() noexcept {}
};

struct TrustedSession : public testing::Test
{
    TrustedSession()
    : set_parent_count(0)
    {
        container.insert_session(mt::fake_shared(trusted_helper));
        container.insert_session(mt::fake_shared(trusted_app1));
        container.insert_session(mt::fake_shared(trusted_app2));

        ON_CALL(trusted_helper, name()).WillByDefault(testing::Return("trusted_helper"));
        ON_CALL(trusted_helper, process_id()).WillByDefault(testing::Return(1));
        ON_CALL(trusted_helper, get_children()).WillByDefault(testing::Return(mt::fake_shared(child_container)));

        ON_CALL(trusted_app1, name()).WillByDefault(testing::Return("trusted_app1"));
        ON_CALL(trusted_app1, process_id()).WillByDefault(testing::Return(2));
        ON_CALL(trusted_app1, set_parent(testing::_)).WillByDefault(Invoke(this, &TrustedSession::set_parent));

        ON_CALL(trusted_app2, name()).WillByDefault(testing::Return("trusted_app2"));
        ON_CALL(trusted_app2, process_id()).WillByDefault(testing::Return(3));
        ON_CALL(trusted_app2, set_parent(testing::_)).WillByDefault(Invoke(this, &TrustedSession::set_parent));
    }

    void set_parent(std::shared_ptr<msh::Session> const&) {
        set_parent_count++;
    }

    ms::DefaultSessionContainer container;
    testing::NiceMock<MockSessionContainer> child_container;
    testing::NiceMock<MockEventSink> sender;

    testing::NiceMock<mtd::MockShellSession> trusted_helper;
    testing::NiceMock<mtd::MockShellSession> trusted_app1;
    testing::NiceMock<mtd::MockShellSession> trusted_app2;
    int set_parent_count;
};
}

TEST_F(TrustedSession, start_and_stop)
{
    using namespace testing;

    EXPECT_CALL(sender, handle_trusted_session_event(_, mir_trusted_session_started)).Times(1);
    EXPECT_CALL(sender, handle_trusted_session_event(_, mir_trusted_session_stopped)).Times(1);

    msh::TrustedSessionCreationParameters parameters;
    auto trusted_session = ms::TrustedSession::start_for(mt::fake_shared(trusted_helper),
                                                      parameters,
                                                      mt::fake_shared(container),
                                                      mt::fake_shared(sender));
    trusted_session->stop();
}

TEST_F(TrustedSession, trusted_session_ids)
{
    using namespace testing;

    msh::TrustedSessionCreationParameters parameters;
    auto trusted_session1 = ms::TrustedSession::start_for(mt::fake_shared(trusted_helper),
                                                      parameters,
                                                      mt::fake_shared(container),
                                                      mt::fake_shared(sender));

    auto trusted_session2 = ms::TrustedSession::start_for(mt::fake_shared(trusted_helper),
                                                      parameters,
                                                      mt::fake_shared(container),
                                                      mt::fake_shared(sender));

    auto trusted_session3 = ms::TrustedSession::start_for(mt::fake_shared(trusted_helper),
                                                      parameters,
                                                      mt::fake_shared(container),
                                                      mt::fake_shared(sender));

    EXPECT_THAT(trusted_session1->id(), Ne(trusted_session2->id()));
    EXPECT_THAT(trusted_session1->id(), Ne(trusted_session3->id()));
    EXPECT_THAT(trusted_session2->id(), Ne(trusted_session3->id()));
}

TEST_F(TrustedSession, parenting)
{
    using namespace testing;

    EXPECT_CALL(trusted_app1, set_parent(_)).Times(1);
    EXPECT_CALL(trusted_app2, set_parent(_)).Times(1);
    EXPECT_CALL(child_container, insert_session(_)).Times(2);

    msh::TrustedSessionCreationParameters parameters;
    auto trusted_session = ms::TrustedSession::start_for(mt::fake_shared(trusted_helper),
                                                      parameters.add_application(trusted_app1.process_id())
                                                                .add_application(trusted_app2.process_id()),
                                                      mt::fake_shared(container),
                                                      mt::fake_shared(sender));

    EXPECT_CALL(child_container, clear()).Times(1);
    trusted_session->stop();
}

