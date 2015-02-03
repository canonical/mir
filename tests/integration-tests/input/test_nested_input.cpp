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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "src/server/report/null/input_report.h"
#include "mir/input/input_manager.h"
#include "mir/input/input_dispatcher.h"
#include "src/server/input/android/android_input_dispatcher.h"
#include "src/server/input/android/event_filter_dispatcher_policy.h"
#include "src/server/input/android/common_input_thread.h"
#include "src/server/report/null_report_factory.h"
#include "mir/raii.h"

#include "mir_test_doubles/stub_scene.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test_doubles/stub_input_enumerator.h"
#include "mir_test/fake_shared.h"
#include "mir_test/event_matchers.h"

#include "InputEnumerator.h"
#include "InputDispatcher.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(NestedInputTest, applies_event_filter_on_relayed_event)
{
    using namespace testing;

    mtd::MockEventFilter mock_event_filter;
    bool repeat_is_disabled = false;
    auto null_report = mir::report::null_input_report();
    mia::EventFilterDispatcherPolicy policy(mt::fake_shared(mock_event_filter), repeat_is_disabled);
    auto  android_dispatcher = std::make_shared<droidinput::InputDispatcher>(mt::fake_shared(policy), null_report, std::make_shared<mtd::StubInputEnumerator>());
    mia::CommonInputThread input_thread("InputDispatcher",
                                        new droidinput::InputDispatcherThread(android_dispatcher));

    mia::AndroidInputDispatcher dispatcher(android_dispatcher, mt::fake_shared(input_thread));

    auto const with_running_input_dispatcher = mir::raii::paired_calls(
        [&] { dispatcher.start();},
        [&] { dispatcher.stop(); });

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

    dispatcher.dispatch(e);
}
