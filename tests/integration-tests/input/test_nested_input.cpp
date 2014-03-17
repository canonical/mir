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
#include "mir/input/input_region.h"
#include "mir/input/cursor_listener.h"
#include "mir/input/input_manager.h"
#include "mir/input/input_targets.h"
#include "src/server/report/null/input_report.h"
#include "mir/geometry/rectangle.h"
#include "mir/raii.h"

#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
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

struct NullInputTargets : mi::InputTargets
{
    void for_each(std::function<void(std::shared_ptr<mi::InputChannel> const&)> const& ) override
    {
    }
};

MATCHER_P(MirKeyEventMatches, event, "")
{
    return event->type == arg.type &&
           event->key.device_id == arg.key.device_id &&
           event->key.source_id == arg.key.source_id &&
           event->key.action == arg.key.action &&
           event->key.flags == arg.key.flags &&
           event->key.modifiers == arg.key.modifiers &&
           event->key.key_code == arg.key.key_code &&
           event->key.scan_code == arg.key.scan_code &&
           event->key.repeat_count == arg.key.repeat_count &&
           event->key.down_time == arg.key.down_time &&
           event->key.event_time == arg.key.event_time &&
           event->key.is_system_key == arg.key.is_system_key;
}
}

TEST(NestedInputTest, applies_event_filter_on_relayed_event)
{
    using namespace testing;

    mi::NestedInputRelay nested_input_relay;
    mtd::MockEventFilter mock_event_filter;

    mi::NestedInputConfiguration input_conf{
        mt::fake_shared(nested_input_relay),
        mt::fake_shared(mock_event_filter),
        std::make_shared<NullInputRegion>(),
        std::make_shared<NullCursorListener>(),
        std::make_shared<mir::report::null::InputReport>()};

    input_conf.set_input_targets(std::make_shared<NullInputTargets>());
    auto const input_manager = input_conf.the_input_manager();

    auto const with_running_input_manager = mir::raii::paired_calls(
        [&] { input_manager->start(); },
        [&] { input_manager->stop(); });

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

    EXPECT_CALL(mock_event_filter, handle(MirKeyEventMatches(&e)))
        .WillOnce(Return(true));

    mi::EventFilter& nested_input_relay_as_filter = nested_input_relay;
    nested_input_relay_as_filter.handle(e);
}
