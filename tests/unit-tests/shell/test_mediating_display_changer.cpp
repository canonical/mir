/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/shell/mediating_display_changer.h"
#include "mir/shell/session_container.h"
#include "mir/graphics/display_configuration_policy.h"
#include "src/server/shell/broadcasting_session_event_sink.h"

#include "mir_test_doubles/mock_display.h"
#include "mir_test_doubles/mock_compositor.h"
#include "mir_test_doubles/null_display_configuration.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test_doubles/mock_shell_session.h"
#include "mir_test_doubles/stub_shell_session.h"
#include "mir_test/fake_shared.h"
#include "mir_test/display_config_matchers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;

class MockDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    ~MockDisplayConfigurationPolicy() noexcept {}
    MOCK_METHOD1(apply_to, void(mg::DisplayConfiguration&));
};

class StubSessionContainer : public msh::SessionContainer
{
public:
    void insert_session(std::shared_ptr<msh::Session> const& session)
    {
        sessions.push_back(session);
    }

    void remove_session(std::shared_ptr<msh::Session> const&)
    {
    }

    void for_each(std::function<void(std::shared_ptr<msh::Session> const&)> f) const
    {
        for (auto const& session : sessions)
            f(session);
    }

private:
    std::vector<std::shared_ptr<msh::Session>> sessions;
};

struct MediatingDisplayChangerTest : public ::testing::Test
{
    MediatingDisplayChangerTest()
    {
        using namespace testing;

        ON_CALL(mock_display, configuration())
            .WillByDefault(Return(mt::fake_shared(base_config)));

        changer = std::make_shared<msh::MediatingDisplayChanger>(
                      mt::fake_shared(mock_display),
                      mt::fake_shared(mock_compositor),
                      mt::fake_shared(mock_conf_policy),
                      mt::fake_shared(stub_session_container),
                      mt::fake_shared(session_event_sink));
    }

    testing::NiceMock<mtd::MockDisplay> mock_display;
    testing::NiceMock<mtd::MockCompositor> mock_compositor;
    testing::NiceMock<MockDisplayConfigurationPolicy> mock_conf_policy;
    StubSessionContainer stub_session_container;
    msh::BroadcastingSessionEventSink session_event_sink;
    mtd::StubDisplayConfig base_config;
    std::shared_ptr<msh::MediatingDisplayChanger> changer;
};

TEST_F(MediatingDisplayChangerTest, returns_active_configuration_from_display)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    EXPECT_CALL(mock_display, configuration())
        .Times(1)
        .WillOnce(Return(mt::fake_shared(conf)));

    auto returned_conf = changer->active_configuration();
    EXPECT_EQ(&conf, returned_conf.get());
}

TEST_F(MediatingDisplayChangerTest, pauses_system_when_applying_new_configuration_for_focused_session)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    auto session = std::make_shared<mtd::StubShellSession>();

    InSequence s;
    EXPECT_CALL(mock_compositor, stop());

    EXPECT_CALL(mock_display, configure(Ref(conf)));

    EXPECT_CALL(mock_compositor, start());

    session_event_sink.handle_focus_change(session);
    changer->configure(session,
                       mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, doesnt_apply_config_for_unfocused_session)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(Ref(conf))).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    changer->configure(std::make_shared<mtd::StubShellSession>(),
                       mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, handles_hardware_change_properly_when_pausing_system)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    InSequence s;
    EXPECT_CALL(mock_conf_policy, apply_to(Ref(conf)));

    EXPECT_CALL(mock_compositor, stop());
    EXPECT_CALL(mock_display, configure(Ref(conf)));
    EXPECT_CALL(mock_compositor, start());

    changer->configure_for_hardware_change(mt::fake_shared(conf),
                                           mir::DisplayChanger::PauseResumeSystem);
}

TEST_F(MediatingDisplayChangerTest, handles_hardware_change_properly_when_retaining_system_state)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    InSequence s;
    EXPECT_CALL(mock_conf_policy, apply_to(Ref(conf)));
    EXPECT_CALL(mock_display, configure(Ref(conf)));

    changer->configure_for_hardware_change(mt::fake_shared(conf),
                                           mir::DisplayChanger::RetainSystemState);
}

TEST_F(MediatingDisplayChangerTest, hardware_change_doesnt_apply_base_config_if_per_session_config_is_active)
{
    using namespace testing;

    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubShellSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    session_event_sink.handle_focus_change(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    InSequence s;
    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    changer->configure_for_hardware_change(conf,
                                           mir::DisplayChanger::PauseResumeSystem);
}

TEST_F(MediatingDisplayChangerTest, notifies_all_sessions_on_hardware_config_change)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    mtd::MockShellSession mock_session1;
    mtd::MockShellSession mock_session2;

    stub_session_container.insert_session(mt::fake_shared(mock_session1));
    stub_session_container.insert_session(mt::fake_shared(mock_session2));

    EXPECT_CALL(mock_session1, send_display_config(_));
    EXPECT_CALL(mock_session2, send_display_config(_));

    changer->configure_for_hardware_change(mt::fake_shared(conf),
                                           mir::DisplayChanger::PauseResumeSystem);
}

TEST_F(MediatingDisplayChangerTest, focusing_a_session_with_attached_config_applies_config)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubShellSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    InSequence s;
    EXPECT_CALL(mock_compositor, stop());
    EXPECT_CALL(mock_display, configure(Ref(*conf)));
    EXPECT_CALL(mock_compositor, start());

    session_event_sink.handle_focus_change(session1);
}

TEST_F(MediatingDisplayChangerTest, focusing_a_session_without_attached_config_applies_base_config)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubShellSession>();
    auto session2 = std::make_shared<mtd::StubShellSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    session_event_sink.handle_focus_change(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    InSequence s;
    EXPECT_CALL(mock_compositor, stop());
    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(base_config))));
    EXPECT_CALL(mock_compositor, start());

    session_event_sink.handle_focus_change(session2);
}

TEST_F(MediatingDisplayChangerTest, losing_focus_applies_base_config)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubShellSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    session_event_sink.handle_focus_change(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    InSequence s;
    EXPECT_CALL(mock_compositor, stop());
    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(base_config))));
    EXPECT_CALL(mock_compositor, start());

    session_event_sink.handle_no_focus();
}

TEST_F(MediatingDisplayChangerTest, base_config_is_not_applied_if_already_active)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubShellSession>();
    auto session2 = std::make_shared<mtd::StubShellSession>();

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    stub_session_container.insert_session(session1);
    stub_session_container.insert_session(session2);

    session_event_sink.handle_focus_change(session1);
    session_event_sink.handle_focus_change(session2);
    session_event_sink.handle_no_focus();
}

TEST_F(MediatingDisplayChangerTest, hardware_change_invalidates_session_configs)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubShellSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    changer->configure_for_hardware_change(conf,
                                           mir::DisplayChanger::PauseResumeSystem);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    /*
     * Session1 had a config, but it should have been invalidated by the hardware
     * change, so expect no reconfiguration.
     */
    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    session_event_sink.handle_focus_change(session1);
}

TEST_F(MediatingDisplayChangerTest, session_stopping_invalidates_session_config)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubShellSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    session_event_sink.handle_session_stopping(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    /*
     * Session1 had a config, but it should have been invalidated by the
     * session stopping event, so expect no reconfiguration.
     */
    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    session_event_sink.handle_focus_change(session1);
}
