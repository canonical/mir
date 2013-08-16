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

#include "mir/shell/mediating_display_changer.h"
#include "mir/shell/session_container.h"
#include "mir/graphics/display_configuration_policy.h"

#include "mir_test_doubles/mock_display.h"
#include "mir_test_doubles/mock_compositor.h"
#include "mir_test_doubles/mock_input_manager.h"
#include "mir_test_doubles/null_display_configuration.h"
#include "mir_test_doubles/mock_shell_session.h"
#include "mir_test/fake_shared.h"

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
        : changer{mt::fake_shared(mock_display),
                  mt::fake_shared(mock_compositor),
                  mt::fake_shared(mock_input_manager),
                  mt::fake_shared(mock_conf_policy),
                  mt::fake_shared(stub_session_container)}
    {
    }

    testing::NiceMock<mtd::MockDisplay> mock_display;
    testing::NiceMock<mtd::MockCompositor> mock_compositor;
    testing::NiceMock<mtd::MockInputManager> mock_input_manager;
    testing::NiceMock<MockDisplayConfigurationPolicy> mock_conf_policy;
    StubSessionContainer stub_session_container;
    msh::MediatingDisplayChanger changer;
};

TEST_F(MediatingDisplayChangerTest, returns_active_configuration_from_display)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    EXPECT_CALL(mock_display, configuration())
        .Times(1)
        .WillOnce(Return(mt::fake_shared(conf)));

    auto returned_conf = changer.active_configuration();
    EXPECT_EQ(&conf, returned_conf.get());
}

TEST_F(MediatingDisplayChangerTest, pauses_system_when_applying_new_configuration)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    InSequence s;
    EXPECT_CALL(mock_compositor, stop());
    EXPECT_CALL(mock_input_manager, stop());

    EXPECT_CALL(mock_display, configure(Ref(conf)));

    EXPECT_CALL(mock_input_manager, start());
    EXPECT_CALL(mock_compositor, start());

    changer.configure({}, mt::fake_shared(conf));
}

TEST_F(MediatingDisplayChangerTest, handles_hardware_change_properly_when_pausing_system)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    InSequence s;
    EXPECT_CALL(mock_conf_policy, apply_to(Ref(conf)));

    EXPECT_CALL(mock_compositor, stop());
    EXPECT_CALL(mock_input_manager, stop());

    EXPECT_CALL(mock_display, configure(Ref(conf)));

    EXPECT_CALL(mock_input_manager, start());
    EXPECT_CALL(mock_compositor, start());

    changer.configure_for_hardware_change(mt::fake_shared(conf),
                                          mir::DisplayChanger::PauseResumeSystem);
}

TEST_F(MediatingDisplayChangerTest, handles_hardware_change_properly_when_retaining_system_state)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    EXPECT_CALL(mock_compositor, stop()).Times(0);
    EXPECT_CALL(mock_input_manager, stop()).Times(0);
    EXPECT_CALL(mock_input_manager, start()).Times(0);
    EXPECT_CALL(mock_compositor, start()).Times(0);

    InSequence s;
    EXPECT_CALL(mock_conf_policy, apply_to(Ref(conf)));
    EXPECT_CALL(mock_display, configure(Ref(conf)));

    changer.configure_for_hardware_change(mt::fake_shared(conf),
                                          mir::DisplayChanger::RetainSystemState);
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

    changer.configure_for_hardware_change(mt::fake_shared(conf),
                                          mir::DisplayChanger::PauseResumeSystem);
}

TEST_F(MediatingDisplayChangerTest, notifies_all_other_sessions_on_session_config_change)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    mtd::MockShellSession mock_session1;
    mtd::MockShellSession mock_session2;
    mtd::MockShellSession mock_session3;

    stub_session_container.insert_session(mt::fake_shared(mock_session1));
    stub_session_container.insert_session(mt::fake_shared(mock_session2));
    stub_session_container.insert_session(mt::fake_shared(mock_session3));

    EXPECT_CALL(mock_session1, send_display_config(_));
    EXPECT_CALL(mock_session2, send_display_config(_)).Times(0);
    EXPECT_CALL(mock_session3, send_display_config(_));

    changer.configure(mt::fake_shared(mock_session2), mt::fake_shared(conf));
}
