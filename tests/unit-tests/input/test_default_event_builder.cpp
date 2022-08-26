/*
 * Copyright © 2022 Canonical Ltd.
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
 */

#include "src/server/input/default_event_builder.h"
#include "mir/cookie/authority.h"

#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;
using namespace std::chrono_literals;

namespace
{

struct DefaultEventBuilder : public Test
{
    mtd::AdvanceableClock clock{{}};
    mir::input::DefaultEventBuilder builder{
        0,
        mt::fake_shared(clock),
        mir::cookie::Authority::create()};

    auto event_timestamp(std::optional<std::chrono::nanoseconds> timestamp) -> std::chrono::nanoseconds
    {
        auto const ev = builder.key_event(timestamp, mir_keyboard_action_down, 0, 0);
        return std::chrono::nanoseconds{mir_input_event_get_event_time(mir_event_get_input_event(ev.get()))};
    }
};
}

TEST_F(DefaultEventBuilder, when_no_timestamp_is_given_then_clock_timestamp_is_used)
{
    clock.advance_by(12s);
    EXPECT_THAT(event_timestamp(std::nullopt), Eq(12s));
}

TEST_F(DefaultEventBuilder, when_no_timestamp_is_given_then_clock_timestamp_is_updated_and_used)
{
    clock.advance_by(12s);
    event_timestamp(std::nullopt);
    clock.advance_by(2s);
    EXPECT_THAT(event_timestamp(std::nullopt), Eq(14s));
}
