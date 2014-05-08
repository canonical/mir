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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/input/nested_input_configuration.h"
#include "src/server/input/nested_input_relay.h"
#include "src/server/report/null/input_report.h"
#include "mir/input/input_region.h"
#include "mir/input/cursor_listener.h"
#include "mir/input/input_manager.h"
#include "mir/input/input_dispatcher.h"
#include "src/server/input/android/input_dispatcher_configuration.h"
#include "src/server/report/null_report_factory.h"
#include "mir/geometry/rectangle.h"
#include "mir/raii.h"

#include "mir_test_doubles/stub_input_targets.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test/fake_shared.h"
#include "mir_test/client_event_matchers.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

namespace
{
struct NullInputRegion : mi::InputRegion
{
    geom::Rectangle bounding_rectangle() override
    {
        return geom::Rectangle();
    }

    void confine(geom::Point& /*point*/) override
    {
    }
};

struct NullCursorListener : mi::CursorListener
{
    void cursor_moved_to(float, float) override {}
};

}

TEST(NestedInputTest, applies_event_filter_on_relayed_event)
{
    using namespace testing;

    mi::NestedInputRelay nested_input_relay;
    mtd::MockEventFilter mock_event_filter;
    mia::InputDispatcherConfiguration input_dispatcher_conf{
        mt::fake_shared(mock_event_filter),
        mir::report::null_input_report()};
    auto const dispatcher = input_dispatcher_conf.the_input_dispatcher();

    mi::NestedInputConfiguration input_conf{
        mt::fake_shared(nested_input_relay),
        mt::fake_shared(input_dispatcher_conf)};

    input_dispatcher_conf.set_input_targets(std::make_shared<mtd::StubInputTargets>());
    auto const input_manager = input_conf.the_input_manager();

    auto const with_running_input_manager = mir::raii::paired_calls(
        [&] { input_manager->start(); dispatcher->start();},
        [&] { dispatcher->stop(); input_manager->stop(); });

    MirEvent e;
    memset(&e, 0, sizeof(MirEvent));
    e.key.type = mir_event_type_key;
    e.key.device_id = 13;
    e.key.source_id = 10;
    e.key.action = mir_key_action_down;
    e.key.flags = static_cast<MirKeyFlag>(0);
    e.key.modifiers = 0;
    e.key.key_code = 81;
    e.key.scan_code = 176;
    e.key.repeat_count = 0;
    e.key.down_time = 1234;
    e.key.event_time = 12345;
    e.key.is_system_key = 0;

    EXPECT_CALL(mock_event_filter, handle(mt::MirKeyEventMatches(&e)))
        .WillOnce(Return(true));

    mi::EventFilter& nested_input_relay_as_filter = nested_input_relay;
    nested_input_relay_as_filter.handle(e);
}
