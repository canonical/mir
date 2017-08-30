/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/active_outputs.h"
#include "miral/output.h"

#include <mir/shell/display_configuration_controller.h>

#include <mir_test_framework/headless_test.h>

#include <mir/test/doubles/fake_display.h>
#include <mir/test/doubles/stub_display_configuration.h>
#include <mir/test/fake_shared.h>
#include <mir/test/signal.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

using namespace miral;
using namespace testing;

namespace
{
struct MockActiveOutputsListener : ActiveOutputsListener
{
    MOCK_METHOD0(advise_output_begin, void());
    MOCK_METHOD0(advise_output_end, void());

    MOCK_METHOD1(advise_output_create, void(Output const&));
    MOCK_METHOD2(advise_output_update, void(Output const&, Output const&));
    MOCK_METHOD1(advise_output_delete, void(Output const&));
};

std::vector<Rectangle> const output_rects{
    {{0,0}, {640,480}},
    {{640,0}, {640,480}}
};

struct ActiveOutputs : mtf::HeadlessTest
{
    ActiveOutputs()
    {
        add_to_environment("MIR_SERVER_NO_FILE", "");
    }

    void SetUp() override
    {
        mtf::HeadlessTest::SetUp();
        preset_display(mt::fake_shared(display));
        active_outputs_monitor(server);
        active_outputs_monitor.add_listener(&active_outputs_listener);
    }

    void TearDown() override
    {
        active_outputs_monitor.delete_listener(&active_outputs_listener);
        mtf::HeadlessTest::TearDown();
    }

    mtd::FakeDisplay display{output_rects};
    ActiveOutputsMonitor active_outputs_monitor;
    NiceMock<MockActiveOutputsListener> active_outputs_listener;

    void update_outputs(std::vector<Rectangle> const& displays)
    {
        mt::Signal signal;
        EXPECT_CALL(active_outputs_listener, advise_output_end()).WillOnce(Invoke([&]{signal.raise(); }));

        mtd::StubDisplayConfig changed_stub_display_config{displays};
        display.emit_configuration_change_event(mt::fake_shared(changed_stub_display_config));

        signal.wait_for(std::chrono::seconds(10));
        ASSERT_TRUE(signal.raised());
    }

    void invert_outputs_in_base_configuration()
    {
        mt::Signal signal;
        EXPECT_CALL(active_outputs_listener, advise_output_end()).WillOnce(Invoke([&]{signal.raise(); }));

        auto configuration = server.the_display()->configuration();
        configuration->for_each_output([](mg::UserDisplayConfigurationOutput& output)
            {
                output.orientation = mir_orientation_inverted;
            });

        server.the_display_configuration_controller()->set_base_configuration(std::move(configuration));

        signal.wait_for(std::chrono::seconds(10));
        ASSERT_TRUE(signal.raised());
    }
};

struct RunServer
{
    RunServer(mtf::HeadlessTest* self) : self{self} { self->start_server(); }
    ~RunServer() { self->stop_server(); }

    mtf::HeadlessTest* const self;
};
}

TEST_F(ActiveOutputs, on_startup_listener_is_advised)
{
    InSequence seq;
    EXPECT_CALL(active_outputs_listener, advise_output_begin());
    EXPECT_CALL(active_outputs_listener, advise_output_create(_)).Times(2);
    RunServer runner{this};

    Mock::VerifyAndClearExpectations(&active_outputs_listener); // before shutdown
}

TEST_F(ActiveOutputs, when_output_unplugged_listener_is_advised)
{
    RunServer runner{this};

    InSequence seq;
    EXPECT_CALL(active_outputs_listener, advise_output_begin());
    EXPECT_CALL(active_outputs_listener, advise_output_delete(_)).Times(1);
    update_outputs({{{0,0}, {640,480}}});

    Mock::VerifyAndClearExpectations(&active_outputs_listener); // before shutdown
}

TEST_F(ActiveOutputs, when_output_added_listener_is_advised)
{
    RunServer runner{this};

    auto new_output_rects = output_rects;
    new_output_rects.emplace_back(Point{1280,0}, Size{640,480});

    InSequence seq;
    EXPECT_CALL(active_outputs_listener, advise_output_begin());
    EXPECT_CALL(active_outputs_listener, advise_output_create(_)).Times(1);
    update_outputs(new_output_rects);

    Mock::VerifyAndClearExpectations(&active_outputs_listener); // before shutdown
}

TEST_F(ActiveOutputs, when_output_resized_listener_is_advised)
{
    RunServer runner{this};

    auto new_output_rects = output_rects;
    new_output_rects[1] = {Point{640,0}, Size{1080,768}};

    InSequence seq;
    EXPECT_CALL(active_outputs_listener, advise_output_begin());
    EXPECT_CALL(active_outputs_listener, advise_output_update(_, _)).Times(1);
    update_outputs(new_output_rects);

    Mock::VerifyAndClearExpectations(&active_outputs_listener); // before shutdown
}

TEST_F(ActiveOutputs, when_base_configuration_is_updated_listener_is_advised)
{
    RunServer runner{this};

    InSequence seq;
    EXPECT_CALL(active_outputs_listener, advise_output_begin());
    EXPECT_CALL(active_outputs_listener, advise_output_update(_, _)).Times(2);
    invert_outputs_in_base_configuration();

    Mock::VerifyAndClearExpectations(&active_outputs_listener); // before shutdown
}

TEST_F(ActiveOutputs, available_to_process)
{
    RunServer runner{this};

    active_outputs_monitor.process_outputs([](std::vector<Output> const& outputs)
        { EXPECT_THAT(outputs.size(), Eq(output_rects.size())); });
}

TEST_F(ActiveOutputs, updates_are_available_to_process)
{
    RunServer runner{this};

    auto new_output_rects = output_rects;
    new_output_rects.emplace_back(Point{1280,0}, Size{640,480});
    update_outputs(new_output_rects);

    active_outputs_monitor.process_outputs([&](std::vector<Output> const& outputs)
        { EXPECT_THAT(outputs.size(), Eq(new_output_rects.size())); });
}
