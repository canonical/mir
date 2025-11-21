/*
 * Copyright © Canonical Ltd.
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

#include <mir/scene/surface_state_tracker.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace ms = mir::scene;

namespace
{
auto precedence(MirWindowState state) -> int
{
    switch (state)
    {
    case mir_window_state_restored: return 1;
    case mir_window_state_horizmaximized: [[fallthrough]];
    case mir_window_state_vertmaximized: return 2;
    case mir_window_state_maximized: return 3;
    case mir_window_state_attached: return 4;
    case mir_window_state_fullscreen: return 5;
    case mir_window_state_minimized: return 6;
    case mir_window_state_hidden: return 7;
    default: throw "Invalid state " + std::to_string(state);
    }
}

auto is_maximized_state(MirWindowState state) -> bool
{
    return
        state == mir_window_state_maximized ||
        state == mir_window_state_vertmaximized ||
        state == mir_window_state_horizmaximized;
}
}

struct SurfaceStateTrackerTestSingle : TestWithParam<MirWindowState>
{
};

struct SurfaceStateTrackerTestDouble : TestWithParam<std::tuple<MirWindowState, MirWindowState>>
{
};

TEST(SurfaceStateTrackerTest, by_default_all_states_false_except_restored)
{
    ms::SurfaceStateTracker tracker{mir_window_state_restored};
    EXPECT_THAT(tracker.has(mir_window_state_hidden), Eq(false));
    EXPECT_THAT(tracker.has(mir_window_state_minimized), Eq(false));
    EXPECT_THAT(tracker.has(mir_window_state_fullscreen), Eq(false));
    EXPECT_THAT(tracker.has(mir_window_state_attached), Eq(false));
    EXPECT_THAT(tracker.has(mir_window_state_horizmaximized), Eq(false));
    EXPECT_THAT(tracker.has(mir_window_state_vertmaximized), Eq(false));
    EXPECT_THAT(tracker.has(mir_window_state_maximized), Eq(false));
    EXPECT_THAT(tracker.has(mir_window_state_restored), Eq(true));
}

TEST(SurfaceStateTrackerTest, with_and_without_return_this)
{
    ms::SurfaceStateTracker tracker{mir_window_state_fullscreen};
    tracker = tracker.with(mir_window_state_minimized).without(mir_window_state_minimized).with(mir_window_state_attached);
    EXPECT_THAT(tracker.active_state(), Eq(mir_window_state_fullscreen));
    EXPECT_THAT(tracker.has(mir_window_state_fullscreen), Eq(true));
    EXPECT_THAT(tracker.has(mir_window_state_minimized), Eq(false));
    EXPECT_THAT(tracker.has(mir_window_state_attached), Eq(true));
}

TEST(SurfaceStateTrackerTest, with_maximized_sets_both_horiz_and_virt)
{
    ms::SurfaceStateTracker tracker{mir_window_state_restored};
    tracker = tracker.with(mir_window_state_maximized);
    EXPECT_THAT(tracker.has(mir_window_state_horizmaximized), Eq(true));
    EXPECT_THAT(tracker.has(mir_window_state_vertmaximized), Eq(true));
}

TEST_P(SurfaceStateTrackerTestSingle, can_create_with_state)
{
    MirWindowState const state = GetParam();
    ms::SurfaceStateTracker tracker{state};
    EXPECT_THAT(tracker.active_state(), Eq(state));
}

TEST_P(SurfaceStateTrackerTestSingle, always_has_state_created_with)
{
    MirWindowState const state = GetParam();
    ms::SurfaceStateTracker tracker{state};
    EXPECT_THAT(tracker.has(state), Eq(true));
}

TEST_P(SurfaceStateTrackerTestDouble, can_set_active_state)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateTracker tracker{a};
    tracker = tracker.with_active_state(b);
    EXPECT_THAT(tracker.active_state(), Eq(b));
}

TEST_P(SurfaceStateTrackerTestDouble, can_set_active_state_multiple_times)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateTracker tracker{a};
    tracker = tracker.with_active_state(b);
    tracker = tracker.with_active_state(a);
    EXPECT_THAT(tracker.active_state(), Eq(a));
}

TEST_P(SurfaceStateTrackerTestDouble, has_state_once_added)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateTracker tracker{a};
    if (b != mir_window_state_restored)
    {
        tracker = tracker.with(b);
        EXPECT_THAT(tracker.has(b), Eq(true));
    }
}

TEST_P(SurfaceStateTrackerTestDouble, does_not_have_state_once_removed)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateTracker tracker{a};
    if (b != mir_window_state_restored)
    {
        tracker = tracker.without(b);
        EXPECT_THAT(tracker.has(b), Eq(false));
    }
}

TEST_P(SurfaceStateTrackerTestDouble, can_add_and_remove_same_state)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateTracker tracker{a};
    if (b != mir_window_state_restored)
    {
        tracker = tracker.with(b);
        tracker = tracker.without(b);
    }
    if (a == b)
    {
        EXPECT_THAT(tracker.active_state(), Eq(mir_window_state_restored));
    }
    else if (a == mir_window_state_maximized && b == mir_window_state_horizmaximized)
    {
        EXPECT_THAT(tracker.active_state(), Eq(mir_window_state_vertmaximized));
    }
    else if (a == mir_window_state_maximized && b == mir_window_state_vertmaximized)
    {
        EXPECT_THAT(tracker.active_state(), Eq(mir_window_state_horizmaximized));
    }
    else if (is_maximized_state(a) && b == mir_window_state_maximized)
    {
        EXPECT_THAT(tracker.active_state(), Eq(mir_window_state_restored));
    }
    else
    {
        EXPECT_THAT(tracker.active_state(), Eq(a));
    }
}

TEST_P(SurfaceStateTrackerTestDouble, has_any_checks_against_list)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateTracker tracker{a};

    bool expected = (a == b) || (a == mir_window_state_fullscreen);
    if (a == mir_window_state_maximized && is_maximized_state(b))
    {
        expected = true;
    }
    if (b == mir_window_state_restored && !is_maximized_state(a))
    {
        expected = true;
    }
    EXPECT_THAT(tracker.has_any({b, mir_window_state_fullscreen}), Eq(expected));
}

TEST_P(SurfaceStateTrackerTestDouble, state_with_highest_precedence_is_active_state)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateTracker tracker{a};
    if (b != mir_window_state_restored)
    {
        tracker = tracker.with(b);
    }
    if (a != b && precedence(a) == 2 && precedence(b) == 2)
    {
        // they are vert and horiz maximized
        EXPECT_THAT(tracker.active_state(), Eq(mir_window_state_maximized));
    }
    else if (precedence(a) > precedence(b))
    {
        EXPECT_THAT(tracker.active_state(), Eq(a));
    }
    else
    {
        EXPECT_THAT(tracker.active_state(), Eq(b));
    }
}

auto const all_states = Values(
    mir_window_state_restored,
    mir_window_state_horizmaximized,
    mir_window_state_vertmaximized,
    mir_window_state_maximized,
    mir_window_state_attached,
    mir_window_state_fullscreen,
    mir_window_state_minimized,
    mir_window_state_hidden);

INSTANTIATE_TEST_SUITE_P(
    SurfaceStateTrackerTestSingle,
    SurfaceStateTrackerTestSingle,
    all_states);

INSTANTIATE_TEST_SUITE_P(
    SurfaceStateTrackerTestDouble,
    SurfaceStateTrackerTestDouble,
    Combine(all_states, all_states));
