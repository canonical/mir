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
#include "mir/graphics/display_configuration_policy.h"

#include "mir_test_doubles/mock_display.h"
#include "mir_test_doubles/mock_compositor.h"
#include "mir_test_doubles/mock_input_manager.h"
#include "mir_test_doubles/null_display_configuration.h"
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

struct MediatingDisplayChangerTest : public ::testing::Test
{
    mtd::MockDisplay mock_display;
    mtd::MockCompositor mock_compositor;
    mtd::MockInputManager mock_input_manager;
    MockDisplayConfigurationPolicy mock_conf_policy;
};

TEST_F(MediatingDisplayChangerTest, returns_active_configuration_from_display)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;

    EXPECT_CALL(mock_display, configuration())
        .Times(1)
        .WillOnce(Return(mt::fake_shared(conf)));

    msh::MediatingDisplayChanger changer(
        mt::fake_shared(mock_display),
        mt::fake_shared(mock_compositor),
        mt::fake_shared(mock_input_manager),
        mt::fake_shared(mock_conf_policy));

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

    msh::MediatingDisplayChanger changer(
        mt::fake_shared(mock_display),
        mt::fake_shared(mock_compositor),
        mt::fake_shared(mock_input_manager),
        mt::fake_shared(mock_conf_policy));

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

    msh::MediatingDisplayChanger changer(
        mt::fake_shared(mock_display),
        mt::fake_shared(mock_compositor),
        mt::fake_shared(mock_input_manager),
        mt::fake_shared(mock_conf_policy));

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

    msh::MediatingDisplayChanger changer(
        mt::fake_shared(mock_display),
        mt::fake_shared(mock_compositor),
        mt::fake_shared(mock_input_manager),
        mt::fake_shared(mock_conf_policy));

    changer.configure_for_hardware_change(mt::fake_shared(conf),
                                          mir::DisplayChanger::RetainSystemState);
}
