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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/shell/input_targeter.h"

#include "clients.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test/cross_process_sync.h"
#include "mir_test/fake_shared.h"
#include "mir_test/event_matchers.h"

#include "mir/compositor/scene.h"
#include "mir/scene/observer.h"
#include "mir/events/event_private.h"

#include "mir_toolkit/event.h"

#include <linux/input.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mt = mir::test;
namespace mc = mir::compositor;
namespace mtf = mir_test_framework;

namespace
{

class CustomMockInputTargeter :
    public msh::InputTargeter
{
public:
    MOCK_METHOD1(set_focus, void(std::shared_ptr<mi::Surface> const& /*focus_surface*/));
    MOCK_METHOD0(clear_focus, void());
};

struct CustomInputTargeterServerConfig : mtf::InputTestingServerConfiguration
{
    std::shared_ptr<msh::InputTargeter> the_input_targeter() override
    {
        auto const targeter = the_input_targeter_mock();

        if (!expectations_set)
        {
            input_targeter_expectations();
            expectations_set = true;
        }

        return targeter;
    }

    std::shared_ptr<::testing::NiceMock<CustomMockInputTargeter>> the_input_targeter_mock()
    {
        return targeter(
            [this]
            {
                return std::make_shared<::testing::NiceMock<CustomMockInputTargeter>>();
            });
    }

    void inject_input() override {}
    virtual void input_targeter_expectations() {}

    mir::CachedPtr<::testing::NiceMock<CustomMockInputTargeter>> targeter;
    mt::CrossProcessSync dispatching_done;
    bool expectations_set{false};
};

}

using CustomInputTargeter = BespokeDisplayServerTestFixture;

TEST_F(CustomInputTargeter, receives_focus_changes)
{
    struct ServerConfig : CustomInputTargeterServerConfig
    {
        void input_targeter_expectations() override
        {
            using namespace ::testing;
            auto const targeter = the_input_targeter_mock();

            EXPECT_CALL(*targeter, set_focus(_)).Times(1)
                .WillOnce(InvokeWithoutArgs([this] { dispatching_done.signal_ready(); }));
        }
    } server_config;

    launch_server_process(server_config);

    mtf::SurfaceCreatingClient client;

    launch_client_process(client);
    run_in_test_process([&]
    {
        server_config.dispatching_done.wait_for_signal_ready_for(std::chrono::seconds{50});
    });
}
