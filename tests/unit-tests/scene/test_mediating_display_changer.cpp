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

#include "src/server/scene/mediating_display_changer.h"
#include "src/server/scene/session_container.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration_report.h"
#include "src/server/scene/broadcasting_session_event_sink.h"
#include "mir/server_action_queue.h"

#include "mir/test/doubles/mock_display.h"
#include "mir/test/doubles/mock_compositor.h"
#include "mir/test/doubles/null_display_configuration.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/mock_scene_session.h"
#include "mir/test/doubles/stub_scene_session.h"
#include "mir/test/fake_shared.h"
#include "mir/test/display_config_matchers.h"

#include <mutex>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;

using namespace testing;

namespace
{

class MockDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    ~MockDisplayConfigurationPolicy() noexcept {}
    MOCK_METHOD1(apply_to, void(mg::DisplayConfiguration&));
};

class StubSessionContainer : public ms::SessionContainer
{
public:
    void insert_session(std::shared_ptr<ms::Session> const& session)
    {
        sessions.push_back(session);
    }

    void remove_session(std::shared_ptr<ms::Session> const&)
    {
    }

    void for_each(std::function<void(std::shared_ptr<ms::Session> const&)> f) const
    {
        for (auto const& session : sessions)
            f(session);
    }

    std::shared_ptr<ms::Session> successor_of(std::shared_ptr<ms::Session> const&) const
    {
        return {};
    }

private:
    std::vector<std::shared_ptr<ms::Session>> sessions;
};

struct MockDisplay : public mtd::MockDisplay
{
    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        conf_ptr = new mtd::StubDisplayConfig{};
        return std::unique_ptr<mg::DisplayConfiguration>(conf_ptr);
    }

    mutable mg::DisplayConfiguration* conf_ptr;
};

struct StubServerActionQueue : mir::ServerActionQueue
{
    void enqueue(void const* /*owner*/, mir::ServerAction const& action) override
    {
        action();
    }

    void pause_processing_for(void const* /*owner*/) override {}
    void resume_processing_for(void const* /*owner*/) override {}
};

struct MockServerActionQueue : mir::ServerActionQueue
{
    MockServerActionQueue()
    {
        ON_CALL(*this, enqueue(_, _)).WillByDefault(InvokeArgument<1>());
    }
    MOCK_METHOD2(enqueue, void(void const*, mir::ServerAction const&));
    MOCK_METHOD1(pause_processing_for, void(void const*));
    MOCK_METHOD1(resume_processing_for, void(void const*));
};

struct StubDisplayConfigurationReport : mg::DisplayConfigurationReport
{
    void initial_configuration(mg::DisplayConfiguration const&) override {}
    void new_configuration(mg::DisplayConfiguration const&) override {}
};


struct MediatingDisplayChangerTest : public ::testing::Test
{
    MediatingDisplayChangerTest()
    {
        using namespace testing;

        changer = std::make_shared<ms::MediatingDisplayChanger>(
                      mt::fake_shared(mock_display),
                      mt::fake_shared(mock_compositor),
                      mt::fake_shared(mock_conf_policy),
                      mt::fake_shared(stub_session_container),
                      mt::fake_shared(session_event_sink),
                      mt::fake_shared(server_action_queue),
                      mt::fake_shared(display_configuration_report));
    }

    testing::NiceMock<MockDisplay> mock_display;
    testing::NiceMock<mtd::MockCompositor> mock_compositor;
    testing::NiceMock<MockDisplayConfigurationPolicy> mock_conf_policy;
    StubSessionContainer stub_session_container;
    ms::BroadcastingSessionEventSink session_event_sink;
    mtd::StubDisplayConfig base_config;
    StubServerActionQueue server_action_queue;
    StubDisplayConfigurationReport display_configuration_report;
    std::shared_ptr<ms::MediatingDisplayChanger> changer;
};

}

TEST_F(MediatingDisplayChangerTest, returns_active_configuration_from_display)
{
    using namespace testing;

    auto returned_conf = changer->active_configuration();
    EXPECT_EQ(mock_display.conf_ptr, returned_conf.get());
}

TEST_F(MediatingDisplayChangerTest, pauses_system_when_applying_new_configuration_for_focused_session)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    auto session = std::make_shared<mtd::StubSceneSession>();

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

    changer->configure(std::make_shared<mtd::StubSceneSession>(),
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
    auto session1 = std::make_shared<mtd::StubSceneSession>();

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
    mtd::MockSceneSession mock_session1;
    mtd::MockSceneSession mock_session2;

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
    auto session1 = std::make_shared<mtd::StubSceneSession>();

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
    auto session1 = std::make_shared<mtd::StubSceneSession>();
    auto session2 = std::make_shared<mtd::StubSceneSession>();

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
    auto session1 = std::make_shared<mtd::StubSceneSession>();

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
    auto session1 = std::make_shared<mtd::StubSceneSession>();
    auto session2 = std::make_shared<mtd::StubSceneSession>();

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
    auto session1 = std::make_shared<mtd::StubSceneSession>();

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
    auto session1 = std::make_shared<mtd::StubSceneSession>();

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

TEST_F(MediatingDisplayChangerTest, uses_server_action_queue_for_configuration_actions)
{
    using namespace testing;

    auto const conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto const session1 = std::make_shared<mtd::StubSceneSession>();
    auto const session2 = std::make_shared<mtd::StubSceneSession>();
    MockServerActionQueue mock_server_action_queue;

    stub_session_container.insert_session(session1);
    stub_session_container.insert_session(session2);

    ms::MediatingDisplayChanger display_changer(
      mt::fake_shared(mock_display),
      mt::fake_shared(mock_compositor),
      mt::fake_shared(mock_conf_policy),
      mt::fake_shared(stub_session_container),
      mt::fake_shared(session_event_sink),
      mt::fake_shared(mock_server_action_queue),
      mt::fake_shared(display_configuration_report));

    void const* owner{nullptr};

    EXPECT_CALL(mock_server_action_queue, enqueue(_, _))
        .WillOnce(DoAll(SaveArg<0>(&owner), InvokeArgument<1>()));
    session_event_sink.handle_focus_change(session1);
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, enqueue(owner, _));
    display_changer.configure(session1, conf);
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, enqueue(owner, _));
    display_changer.configure_for_hardware_change(
        conf,
        mir::DisplayChanger::PauseResumeSystem);
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, enqueue(owner, _));
    session_event_sink.handle_focus_change(session2);
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, enqueue(owner, _));
    session_event_sink.handle_no_focus();
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, pause_processing_for(owner));
    display_changer.pause_display_config_processing();
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, resume_processing_for(owner));
    display_changer.resume_display_config_processing();
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);
}

TEST_F(MediatingDisplayChangerTest, does_not_block_IPC_thread_for_inactive_sessions)
{
    using namespace testing;

    auto const conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto const active_session = std::make_shared<mtd::StubSceneSession>();
    auto const inactive_session = std::make_shared<mtd::StubSceneSession>();
    MockServerActionQueue mock_server_action_queue;

    stub_session_container.insert_session(active_session);
    stub_session_container.insert_session(inactive_session);

    ms::MediatingDisplayChanger display_changer(
        mt::fake_shared(mock_display),
        mt::fake_shared(mock_compositor),
        mt::fake_shared(mock_conf_policy),
        mt::fake_shared(stub_session_container),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(mock_server_action_queue),
        mt::fake_shared(display_configuration_report));

    EXPECT_CALL(mock_server_action_queue, enqueue(_, _));
    session_event_sink.handle_focus_change(active_session);
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, enqueue(_, _)).Times(0);

    display_changer.configure(inactive_session, conf);
}

TEST_F(MediatingDisplayChangerTest, set_default_display_configuration_doesnt_override_session_configuration)
{
    using namespace testing;

    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSceneSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    session_event_sink.handle_focus_change(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(_)).Times(0);

    changer->set_default_display_configuration(conf);
}

TEST_F(MediatingDisplayChangerTest, set_default_display_configuration_overrides_base_configuration)
{
    using namespace testing;

    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSceneSession>();

    stub_session_container.insert_session(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(_)).Times(1);

    changer->set_default_display_configuration(conf);
}

TEST_F(MediatingDisplayChangerTest, stores_new_base_config_on_set_default_configuration)
{
    using namespace testing;

    auto default_conf = std::make_shared<mtd::StubDisplayConfig>(2);
    auto session_conf = std::make_shared<mtd::StubDisplayConfig>(4);

    auto mock_session1 = std::make_shared<NiceMock<mtd::MockSceneSession>>();
    auto mock_session2 = std::make_shared<NiceMock<mtd::MockSceneSession>>();
    auto mock_session3 = std::make_shared<NiceMock<mtd::MockSceneSession>>();

    stub_session_container.insert_session(mock_session1);

    stub_session_container.insert_session(mock_session2);
    session_event_sink.handle_focus_change(mock_session2);
    changer->configure(mock_session2, session_conf);

    stub_session_container.insert_session(mock_session3);


    Mock::VerifyAndClearExpectations(&mock_display);

    InSequence s;
    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(*default_conf))));
    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(*session_conf))));
    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(*default_conf))));

    changer->set_default_display_configuration(default_conf);

    session_event_sink.handle_focus_change(mock_session1);
    session_event_sink.handle_focus_change(mock_session2);
    session_event_sink.handle_focus_change(mock_session3);
}
