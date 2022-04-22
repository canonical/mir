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

#include "src/server/frontend_wayland/wayland_timespec.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace mt = mir::time;
using namespace std::chrono_literals;

using namespace testing;

TEST(WaylandTimespec, zero_is_correct)
{
    mf::WaylandTimespec ts{mt::Timestamp{0s}};
    EXPECT_THAT(ts.tv_sec_hi, Eq(0));
    EXPECT_THAT(ts.tv_sec_lo, Eq(0));
    EXPECT_THAT(ts.tv_nsec, Eq(0));
}

TEST(WaylandTimespec, handles_nanoseconds)
{
    mf::WaylandTimespec ts{mt::Timestamp{12ns}};
    EXPECT_THAT(ts.tv_sec_hi, Eq(0));
    EXPECT_THAT(ts.tv_sec_lo, Eq(0));
    EXPECT_THAT(ts.tv_nsec, Eq(12));
}

TEST(WaylandTimespec, handles_seconds)
{
    mf::WaylandTimespec ts{mt::Timestamp{17s}};
    EXPECT_THAT(ts.tv_sec_hi, Eq(0));
    EXPECT_THAT(ts.tv_sec_lo, Eq(17));
    EXPECT_THAT(ts.tv_nsec, Eq(0));
}

// Times large enough to require high bits are not supported by mir::Timestamp
/*
TEST(WaylandTimespec, handles_seconds_high_bits)
{
    mf::WaylandTimespec ts{mt::Timestamp{2160368549888s}};
    EXPECT_THAT(ts.tv_sec_hi, Eq(502));
    EXPECT_THAT(ts.tv_sec_lo, Eq(0));
    EXPECT_THAT(ts.tv_nsec, Eq(0));
}

TEST(WaylandTimespec, handles_all)
{
    mf::WaylandTimespec ts{mt::Timestamp{77309411431s + 54ns}};
    EXPECT_THAT(ts.tv_sec_hi, Eq(18));
    EXPECT_THAT(ts.tv_sec_lo, Eq(103));
    EXPECT_THAT(ts.tv_nsec, Eq(54));
}
*/
