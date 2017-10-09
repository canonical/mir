/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/scene/mediating_display_changer.h"
#include "mir/scene/session_container.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration_observer.h"
#include "mir/geometry/rectangles.h"
#include "src/server/scene/broadcasting_session_event_sink.h"
#include "mir/server_action_queue.h"

#include "mir/test/doubles/mock_display.h"
#include "mir/test/doubles/mock_compositor.h"
#include "mir/test/doubles/null_display_configuration.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/mock_scene_session.h"
#include "mir/test/doubles/stub_session.h"
#include "mir/test/doubles/stub_session_container.h"
#include "mir/test/fake_shared.h"
#include "mir/test/display_config_matchers.h"
#include "mir/test/doubles/fake_alarm_factory.h"

#include <mutex>
#include <boost/throw_exception.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

using namespace testing;

namespace
{

struct TestDisplayConfiguration : mtd::NullDisplayConfiguration
{
    std::vector<mg::DisplayConfigurationOutput> const outputs;
    TestDisplayConfiguration(std::vector<mg::DisplayConfigurationOutput> const& items)
        : outputs{items} {}
    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> fun) const override
    {
        for (auto const& output : outputs)
            fun(output);
    }
    std::unique_ptr<mg::DisplayConfiguration> clone() const override
    {
        return std::make_unique<TestDisplayConfiguration>(outputs);
    }
};

class MockDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    ~MockDisplayConfigurationPolicy() noexcept {}
    MOCK_METHOD1(apply_to, void(mg::DisplayConfiguration&));
};


struct MockDisplay : public mtd::MockDisplay
{
    MockDisplay()
        : config{std::make_unique<mtd::StubDisplayConfig>()}
    {
        ON_CALL(*this, configure(_))
            .WillByDefault(Invoke([this](auto& conf) { config = conf.clone(); }));
    }

    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        return config->clone();
    }

    std::unique_ptr<mg::DisplayConfiguration> config;
};

struct StubServerActionQueue : mir::ServerActionQueue
{
    void enqueue(void const* /*owner*/, mir::ServerAction const& action) override
    {
        action();
    }
    void enqueue_with_guaranteed_execution(mir::ServerAction const& action) override
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
        ON_CALL(*this, enqueue_with_guaranteed_execution(_)).WillByDefault(InvokeArgument<0>());
    }
    MOCK_METHOD2(enqueue, void(void const*, mir::ServerAction const&));
    MOCK_METHOD1(enqueue_with_guaranteed_execution, void(mir::ServerAction const&));
    MOCK_METHOD1(pause_processing_for, void(void const*));
    MOCK_METHOD1(resume_processing_for, void(void const*));
};

struct StubDisplayConfigurationObserver : mg::DisplayConfigurationObserver
{
    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const&) override {}
    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const&) override {}
    void base_configuration_updated(std::shared_ptr<mg::DisplayConfiguration const> const&) override {}
    void session_configuration_applied(
        std::shared_ptr<mf::Session> const&,
        std::shared_ptr<mg::DisplayConfiguration> const&) override {};
    void session_configuration_removed(std::shared_ptr<mf::Session> const&) override {};
    void configuration_failed(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override {}
    void catastrophic_configuration_error(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override { }
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
                      mt::fake_shared(display_configuration_report),
                      mt::fake_shared(alarm_factory));
    }

    testing::NiceMock<MockDisplay> mock_display;
    testing::NiceMock<mtd::MockCompositor> mock_compositor;
    testing::NiceMock<MockDisplayConfigurationPolicy> mock_conf_policy;
    mtd::StubSessionContainer stub_session_container;
    ms::BroadcastingSessionEventSink session_event_sink;
    mtd::StubDisplayConfig base_config;
    StubServerActionQueue server_action_queue;
    StubDisplayConfigurationObserver display_configuration_report;
    mtd::FakeAlarmFactory alarm_factory;
    std::shared_ptr<ms::MediatingDisplayChanger> changer;
};

}

TEST_F(MediatingDisplayChangerTest, returns_base_configuration_from_display_at_startup)
{
    using namespace testing;

    auto const base_conf = changer->base_configuration();
    EXPECT_THAT(*base_conf, mt::DisplayConfigMatches(std::ref(*mock_display.configuration())));
}

TEST_F(MediatingDisplayChangerTest, pauses_system_when_applying_new_configuration_for_focused_session_would_invalidate_display_buffers)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    auto session = std::make_shared<mtd::StubSession>();

    ON_CALL(mock_display, apply_if_configuration_preserves_display_buffers(_))
        .WillByDefault(Return(false));

    InSequence s;
    EXPECT_CALL(mock_compositor, stop());

    EXPECT_CALL(mock_display, configure(Ref(conf)));

    EXPECT_CALL(mock_compositor, start());

    session_event_sink.handle_focus_change(session);
    changer->configure(session,
                       mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, does_not_pause_system_when_applying_new_configuration_for_focused_session_would_preserve_display_buffers)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    auto session = std::make_shared<mtd::StubSession>();

    EXPECT_CALL(mock_display, apply_if_configuration_preserves_display_buffers(Ref(conf)))
        .WillOnce(Return(true));

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);

    session_event_sink.handle_focus_change(session);
    changer->configure(session,
                       mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, sends_error_when_applying_new_configuration_for_focused_session_fails)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    auto session = std::make_shared<mtd::MockSceneSession>();

    ON_CALL(mock_display, configure(Ref(conf)))
        .WillByDefault(InvokeWithoutArgs([]() { BOOST_THROW_EXCEPTION(std::runtime_error{"Ducks!"}); }));
    EXPECT_CALL(*session, send_error(_));

    session_event_sink.handle_focus_change(session);
    changer->configure(session,
                       mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, reapplies_old_config_when_applying_new_configuration_fails)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    auto session = std::make_shared<NiceMock<mtd::MockSceneSession>>();
    auto existing_configuration = changer->base_configuration();

    InSequence s;
    EXPECT_CALL(mock_display, configure(Ref(conf)))
        .WillOnce(InvokeWithoutArgs([]() { BOOST_THROW_EXCEPTION(std::runtime_error{"Ducks!"}); }));
    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(*existing_configuration))));

    session_event_sink.handle_focus_change(session);
    changer->configure(session, mt::fake_shared(conf));
}


TEST_F(MediatingDisplayChangerTest, doesnt_apply_config_for_unfocused_session)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(Ref(conf))).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    changer->configure(std::make_shared<mtd::StubSession>(),
                       mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, returns_updated_base_configuration_after_hardware_change)
{
    using namespace testing;

    mtd::StubDisplayConfig conf{2};

    changer->configure_for_hardware_change(
        mt::fake_shared(conf));

    auto const base_conf = changer->base_configuration();
    EXPECT_THAT(*base_conf, mt::DisplayConfigMatches(conf));
}

TEST_F(MediatingDisplayChangerTest, handles_hardware_change_when_display_buffers_are_invalidated)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    ON_CALL(mock_display, apply_if_configuration_preserves_display_buffers(_))
        .WillByDefault(Return(false));

    InSequence s;
    EXPECT_CALL(mock_conf_policy, apply_to(Ref(conf)));

    EXPECT_CALL(mock_compositor, stop());
    EXPECT_CALL(mock_display, configure(Ref(conf)));
    EXPECT_CALL(mock_compositor, start());

    changer->configure_for_hardware_change(mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, handles_error_when_applying_hardware_change)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;


    EXPECT_CALL(mock_display, configure(Ref(conf)))
        .WillOnce(InvokeWithoutArgs([]() { BOOST_THROW_EXCEPTION(std::runtime_error{"Avocado!"}); }));
    EXPECT_CALL(mock_display, configure(Not(Ref(conf))))
        .Times(AnyNumber());

    auto const previous_base_config = changer->base_configuration();
    ASSERT_THAT(conf, Not(mt::DisplayConfigMatches(std::cref(*previous_base_config))));

    changer->configure_for_hardware_change(mt::fake_shared(conf));

    EXPECT_THAT(*changer->base_configuration(), mt::DisplayConfigMatches(std::cref(*previous_base_config)));
}

TEST_F(MediatingDisplayChangerTest, handles_hardware_change_when_display_buffers_are_preserved)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    {
        InSequence s;
        EXPECT_CALL(mock_conf_policy, apply_to(Ref(conf)));

        EXPECT_CALL(mock_display, apply_if_configuration_preserves_display_buffers(Ref(conf)))
            .WillOnce(Return(true));
    }

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    changer->configure_for_hardware_change(mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, handles_hardware_change_when_display_buffers_are_preserved_but_new_outputs_are_enabled)
{
    using namespace testing;

    auto conf = changer->base_configuration();

    bool new_output_enabled{false};
    conf->for_each_output(
        [&new_output_enabled](mg::UserDisplayConfigurationOutput& output)
        {
            if (!output.used)
            {
                new_output_enabled = true;
                output.used = true;
            }
        });
    ASSERT_TRUE(new_output_enabled);

    ON_CALL(mock_display, apply_if_configuration_preserves_display_buffers(_))
        .WillByDefault(Return(true));

    InSequence s;
    EXPECT_CALL(mock_conf_policy, apply_to(Ref(*conf)));

    /*
     * Currently we have to tear down and recreate the compositor in order to add
     * a new output. This is not an inherent limitation, and we might do better later.
     */
    EXPECT_CALL(mock_compositor, stop()).Times(1);
    EXPECT_CALL(mock_display, configure(Ref(*conf)));
    EXPECT_CALL(mock_compositor, start()).Times(1);

    changer->configure_for_hardware_change(conf);
}

TEST_F(MediatingDisplayChangerTest, hardware_change_doesnt_apply_base_config_if_per_session_config_is_active)
{
    using namespace testing;

    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    session_event_sink.handle_focus_change(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    InSequence s;
    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    changer->configure_for_hardware_change(conf);
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

    changer->configure_for_hardware_change(mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, notifies_all_sessions_when_hardware_config_change_fails)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    mtd::MockSceneSession mock_session1;
    mtd::MockSceneSession mock_session2;

    auto const previous_base_config = changer->base_configuration();
    ASSERT_THAT(conf, Not(mt::DisplayConfigMatches(std::cref(*previous_base_config))));

    stub_session_container.insert_session(mt::fake_shared(mock_session1));
    stub_session_container.insert_session(mt::fake_shared(mock_session2));

    EXPECT_CALL(mock_display, configure(Ref(conf)))
        .WillOnce(InvokeWithoutArgs([]() { BOOST_THROW_EXCEPTION(std::runtime_error{"Avocado!"}); }));
    EXPECT_CALL(mock_display, configure(Not(Ref(conf))))
        .Times(AnyNumber());
    EXPECT_CALL(mock_session1, send_display_config(mt::DisplayConfigMatches(std::cref(*previous_base_config))));
    EXPECT_CALL(mock_session2, send_display_config(mt::DisplayConfigMatches(std::cref(*previous_base_config))));

    changer->configure_for_hardware_change(mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, focusing_a_session_with_db_preserving_attached_config_applies_config)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    EXPECT_CALL(mock_display, apply_if_configuration_preserves_display_buffers(Ref(*conf)))
        .WillOnce(Return(true));

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    session_event_sink.handle_focus_change(session1);
}

TEST_F(MediatingDisplayChangerTest, focusing_a_session_with_db_preserving_but_output_adding_attached_config_applies_config)
{
    using namespace testing;
    auto session1 = std::make_shared<mtd::StubSession>();

    auto conf = changer->base_configuration();

    bool new_output_enabled{false};
    conf->for_each_output(
        [&new_output_enabled](mg::UserDisplayConfigurationOutput& output)
        {
            if (output.connected && !output.used)
            {
                new_output_enabled = true;
                output.used = true;
            }
        });
    ASSERT_TRUE(new_output_enabled);

    ON_CALL(mock_display, apply_if_configuration_preserves_display_buffers(_))
        .WillByDefault(Return(true));

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    /*
     * Currently we have to tear down and recreate the compositor in order to add
     * a new output. This is not an inherent limitation, and we might do better later.
     */
    InSequence s;
    EXPECT_CALL(mock_compositor, stop()).Times(1);
    EXPECT_CALL(mock_display, configure(Ref(*conf)));
    EXPECT_CALL(mock_compositor, start()).Times(1);

    session_event_sink.handle_focus_change(session1);
}

TEST_F(MediatingDisplayChangerTest, focusing_a_session_with_db_invalidating_attached_config_applies_config)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSession>();

    ON_CALL(mock_display, apply_if_configuration_preserves_display_buffers(_))
        .WillByDefault(Return(false));

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    InSequence s;
    EXPECT_CALL(mock_compositor, stop());
    EXPECT_CALL(mock_display, configure(Ref(*conf)));
    EXPECT_CALL(mock_compositor, start());

    session_event_sink.handle_focus_change(session1);
}

TEST_F(MediatingDisplayChangerTest, failure_to_apply_session_config_on_focus_sends_error)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::MockSceneSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    EXPECT_CALL(
        mock_display,
        configure(mt::DisplayConfigMatches(std::cref(*conf))))
            .WillOnce(InvokeWithoutArgs([]() { BOOST_THROW_EXCEPTION(std::runtime_error{"Banana"}); }));
    EXPECT_CALL(
        mock_display,
        configure(Not(mt::DisplayConfigMatches(std::cref(*conf)))))
            .Times(AnyNumber());
    EXPECT_CALL(*session1, send_error(_));

    session_event_sink.handle_focus_change(session1);
}

TEST_F(MediatingDisplayChangerTest, focusing_a_session_without_attached_config_applies_base_config_pausing_if_db_invalidated)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSession>();
    auto session2 = std::make_shared<mtd::StubSession>();

    ON_CALL(mock_display, apply_if_configuration_preserves_display_buffers(_))
        .WillByDefault(Return(false));

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

TEST_F(MediatingDisplayChangerTest, focusing_a_session_without_attached_config_applies_base_config_not_pausing_if_db_preserved)
{
    using namespace testing;
    std::shared_ptr<mg::DisplayConfiguration> conf = base_config.clone();
    conf->for_each_output(
        [](mg::UserDisplayConfigurationOutput& output)
        {
            if (output.used)
            {
                output.orientation =
                    output.orientation == mir_orientation_normal ? mir_orientation_left : mir_orientation_normal;
            }
        });

    auto session1 = std::make_shared<mtd::StubSession>();
    auto session2 = std::make_shared<mtd::StubSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    session_event_sink.handle_focus_change(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(
        mock_display,
        apply_if_configuration_preserves_display_buffers(mt::DisplayConfigMatches(std::cref(base_config))))
            .WillOnce(Return(true));

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    session_event_sink.handle_focus_change(session2);
}

TEST_F(MediatingDisplayChangerTest, losing_focus_applies_base_config)
{
    using namespace testing;
    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSession>();

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
    auto session1 = std::make_shared<mtd::StubSession>();
    auto session2 = std::make_shared<mtd::StubSession>();

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
    auto session1 = std::make_shared<mtd::StubSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    changer->configure_for_hardware_change(conf);

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
    auto session1 = std::make_shared<mtd::StubSession>();

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
    auto const session1 = std::make_shared<mtd::StubSession>();
    auto const session2 = std::make_shared<mtd::StubSession>();
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
      mt::fake_shared(display_configuration_report),
      mt::fake_shared(alarm_factory));

    void const* owner{nullptr};

    EXPECT_CALL(mock_server_action_queue, enqueue(_, _))
        .WillOnce(DoAll(SaveArg<0>(&owner), InvokeArgument<1>()));
    session_event_sink.handle_focus_change(session1);
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, enqueue(owner, _));
    display_changer.configure(session1, conf);
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, enqueue(owner, _));
    display_changer.configure_for_hardware_change(conf);
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
    auto const active_session = std::make_shared<mtd::StubSession>();
    auto const inactive_session = std::make_shared<mtd::StubSession>();
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
        mt::fake_shared(display_configuration_report),
        mt::fake_shared(alarm_factory));

    EXPECT_CALL(mock_server_action_queue, enqueue(_, _));
    session_event_sink.handle_focus_change(active_session);
    Mock::VerifyAndClearExpectations(&mock_server_action_queue);

    EXPECT_CALL(mock_server_action_queue, enqueue(_, _)).Times(0);

    display_changer.configure(inactive_session, conf);
}

TEST_F(MediatingDisplayChangerTest, set_base_configuration_doesnt_override_session_configuration)
{
    using namespace testing;

    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSession>();

    stub_session_container.insert_session(session1);
    changer->configure(session1, conf);

    session_event_sink.handle_focus_change(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(_)).Times(0);

    changer->set_base_configuration(conf);
}

TEST_F(MediatingDisplayChangerTest, set_base_configuration_overrides_base_configuration)
{
    using namespace testing;

    auto conf = std::make_shared<mtd::NullDisplayConfiguration>();
    auto session1 = std::make_shared<mtd::StubSession>();

    stub_session_container.insert_session(session1);

    Mock::VerifyAndClearExpectations(&mock_compositor);
    Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(_)).Times(1);

    changer->set_base_configuration(conf);
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

    changer->set_base_configuration(default_conf);

    session_event_sink.handle_focus_change(mock_session1);
    session_event_sink.handle_focus_change(mock_session2);
    session_event_sink.handle_focus_change(mock_session3);
}

TEST_F(MediatingDisplayChangerTest,
       returns_updated_base_configuration_after_set_base_configuration)
{
    using namespace testing;

    mtd::StubDisplayConfig conf{2};

    changer->set_base_configuration(mt::fake_shared(conf));

    auto const base_conf = changer->base_configuration();
    EXPECT_THAT(*base_conf, mt::DisplayConfigMatches(conf));
}

TEST_F(MediatingDisplayChangerTest, notifies_all_sessions_on_set_base_configuration)
{
    using namespace testing;

    mtd::NullDisplayConfiguration conf;
    mtd::MockSceneSession mock_session1;
    mtd::MockSceneSession mock_session2;

    stub_session_container.insert_session(mt::fake_shared(mock_session1));
    stub_session_container.insert_session(mt::fake_shared(mock_session2));

    EXPECT_CALL(mock_session1, send_display_config(_));
    EXPECT_CALL(mock_session2, send_display_config(_));

    changer->set_base_configuration(mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, notifies_observer_on_set_base_configuration)
{
    using namespace testing;

    struct MockDisplayConfigurationObserver : StubDisplayConfigurationObserver
    {
        MOCK_METHOD1(base_configuration_updated, void (std::shared_ptr<mg::DisplayConfiguration const> const& base_config));
    } display_configuration_observer;

    changer = std::make_shared<ms::MediatingDisplayChanger>(
        mt::fake_shared(mock_display),
        mt::fake_shared(mock_compositor),
        mt::fake_shared(mock_conf_policy),
        mt::fake_shared(stub_session_container),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(server_action_queue),
        mt::fake_shared(display_configuration_observer),
        mt::fake_shared(alarm_factory));

    mtd::NullDisplayConfiguration conf;

    EXPECT_CALL(display_configuration_observer, base_configuration_updated(_));

    changer->set_base_configuration(mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, notifies_session_on_preview_base_configuration)
{
    using namespace testing;

    mtd::NullDisplayConfiguration conf;
    auto const mock_session = std::make_shared<NiceMock<mtd::MockSceneSession>>();

    stub_session_container.insert_session(mock_session);

    EXPECT_CALL(*mock_session, send_display_config(_));

    changer->preview_base_configuration(
        mock_session,
        mt::fake_shared(conf),
        std::chrono::seconds{1});
}

TEST_F(MediatingDisplayChangerTest, reverts_to_previous_configuration_on_timeout)
{
    using namespace testing;

    auto new_config = std::make_shared<mtd::StubDisplayConfig>(1);
    auto old_config = changer->base_configuration();

    auto applied_config = old_config->clone();

    ON_CALL(mock_display, configure(_))
        .WillByDefault(Invoke([&applied_config](auto& conf) { applied_config = conf.clone(); }));

    auto mock_session = std::make_shared<NiceMock<mtd::MockSceneSession>>();

    stub_session_container.insert_session(mock_session);

    std::chrono::seconds const timeout{30};

    changer->preview_base_configuration(
        mock_session,
        new_config,
        timeout);

    EXPECT_THAT(*applied_config, mt::DisplayConfigMatches(std::cref(*new_config)));

    alarm_factory.advance_smoothly_by(timeout - std::chrono::milliseconds{1});
    EXPECT_THAT(*applied_config, mt::DisplayConfigMatches(std::cref(*new_config)));

    alarm_factory.advance_smoothly_by(std::chrono::milliseconds{2});
    alarm_factory.advance_smoothly_by(timeout);
    EXPECT_THAT(*applied_config, mt::DisplayConfigMatches(std::cref(*old_config)));
}

TEST_F(MediatingDisplayChangerTest, only_configuring_client_receives_preview_notifications)
{
    using namespace testing;

    mtd::NullDisplayConfiguration conf;
    auto const mock_session1 = std::make_shared<NiceMock<mtd::MockSceneSession>>();
    auto const mock_session2 = std::make_shared<NiceMock<mtd::MockSceneSession>>();

    auto new_config = std::make_shared<mtd::StubDisplayConfig>(1);
    auto old_config = changer->base_configuration();

    stub_session_container.insert_session(mock_session1);
    stub_session_container.insert_session(mock_session2);

    EXPECT_CALL(*mock_session2, send_display_config(_)).Times(0);

    std::chrono::seconds const timeout{30};

    changer->preview_base_configuration(
        mock_session1,
        new_config,
        timeout);

    alarm_factory.advance_smoothly_by(timeout + std::chrono::seconds{1});
}

TEST_F(MediatingDisplayChangerTest, only_one_client_can_preview_configuration_change_at_once)
{
    using namespace testing;

    auto const mock_session1 = std::make_shared<NiceMock<mtd::MockSceneSession>>();
    auto const mock_session2 = std::make_shared<NiceMock<mtd::MockSceneSession>>();

    auto new_config = std::make_shared<mtd::StubDisplayConfig>(1);

    stub_session_container.insert_session(mock_session1);
    stub_session_container.insert_session(mock_session2);

    std::chrono::seconds const timeout{30};

    changer->preview_base_configuration(
        mock_session1,
        new_config,
        timeout);

    EXPECT_THROW(
        {
            changer->preview_base_configuration(
                mock_session2,
                new_config,
                timeout);
        },
        std::runtime_error);
}

TEST_F(MediatingDisplayChangerTest, confirmed_configuration_doesnt_revert_after_timeout)
{
    using namespace testing;

    auto new_config = std::make_shared<mtd::StubDisplayConfig>(1);
    auto old_config = changer->base_configuration();

    auto applied_config = old_config->clone();

    ASSERT_THAT(applied_config, Not(Eq(nullptr)));

    ON_CALL(mock_display, configure(_))
        .WillByDefault(Invoke([&applied_config](auto& conf) { applied_config = conf.clone(); }));

    auto mock_session = std::make_shared<NiceMock<mtd::MockSceneSession>>();

    stub_session_container.insert_session(mock_session);

    std::chrono::seconds const timeout{30};

    changer->preview_base_configuration(
        mock_session,
        new_config,
        timeout);

    EXPECT_THAT(*applied_config, mt::DisplayConfigMatches(std::cref(*new_config)));

    changer->confirm_base_configuration(mock_session, new_config);

    alarm_factory.advance_smoothly_by(timeout * 2);
    EXPECT_THAT(*applied_config, mt::DisplayConfigMatches(std::cref(*new_config)));
}

TEST_F(MediatingDisplayChangerTest, all_sessions_get_notified_on_configuration_confirmation)
{
    using namespace testing;

    mtd::NullDisplayConfiguration conf;
    auto const mock_session1 = std::make_shared<NiceMock<mtd::MockSceneSession>>();
    auto const mock_session2 = std::make_shared<NiceMock<mtd::MockSceneSession>>();

    auto new_config = std::make_shared<mtd::StubDisplayConfig>(1);
    auto old_config = changer->base_configuration();

    stub_session_container.insert_session(mock_session1);
    stub_session_container.insert_session(mock_session2);

    std::unique_ptr<mg::DisplayConfiguration> received_configuration;

    ON_CALL(*mock_session2, send_display_config(_))
        .WillByDefault(Invoke([&received_configuration](auto& conf) { received_configuration = conf.clone(); }));

    changer->preview_base_configuration(
        mock_session1,
        new_config,
        std::chrono::seconds{1});

    EXPECT_THAT(received_configuration, Eq(nullptr));

    changer->confirm_base_configuration(mock_session1, new_config);

    ASSERT_THAT(received_configuration, Not(Eq(nullptr)));
    EXPECT_THAT(*received_configuration, mt::DisplayConfigMatches(std::cref(*new_config)));
}

