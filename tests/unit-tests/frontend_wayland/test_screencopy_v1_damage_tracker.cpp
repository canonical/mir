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

#include "src/server/frontend_wayland/wlr_screencopy_v1.h"
#include "mir/scene/observer.h"
#include "mir/scene/surface_observer.h"
#include "mir/test/doubles/mock_frontend_surface_stack.h"
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
struct MockFrame : mf::WlrScreencopyV1DamageTracker::Frame, mw::LifetimeTracker
{
public:
    MockFrame(geom::Rectangle output_space_area)
        : params{nullptr, output_space_area, output_space_area.size}
    {
    }

    MockFrame(geom::Rectangle output_space_area, geom::Size buffer_size)
        : params{nullptr, output_space_area, buffer_size}
    {
    }

    auto destroyed_flag() const -> std::shared_ptr<bool const> override
    {
        return LifetimeTracker::destroyed_flag();
    }

    auto parameters() const -> mf::WlrScreencopyV1DamageTracker::FrameParams const& override
    {
        return params;
    }

    MOCK_METHOD(void, capture, (geom::Rectangle damage), (override));

    mf::WlrScreencopyV1DamageTracker::FrameParams params;
};

struct DamageTrackerV1 : Test
{
    NiceMock<mtd::MockSurface> surface;
    NiceMock<mtd::MockFrontendSurfaceStack> surface_stack;
    mtd::ExplicitExecutor executor;
    mf::WlrScreencopyV1DamageTracker tracker{executor, surface_stack};
    std::shared_ptr<ms::Observer> scene_observer;
    std::weak_ptr<ms::SurfaceObserver> surface_observer;

    void SetUp() override
    {
        ON_CALL(surface_stack, add_observer(_)).WillByDefault(Invoke([this]
            (std::shared_ptr<ms::Observer>  const& observer)
            {
                scene_observer = observer;
                observer->surface_exists(mt::fake_shared(surface));
            }));
        ON_CALL(surface, register_interest(_)).WillByDefault(SaveArg<0>(&surface_observer));
    }

    void TearDown() override
    {
        executor.execute();
    }

    void capture_frame(geom::Rectangle area, geom::Size buffer_size)
    {
        NiceMock<MockFrame> frame{area, buffer_size};
        tracker.capture_on_damage(&frame);
        executor.execute();
    }

    void damage_whole_scene()
    {
        ASSERT_THAT(scene_observer, NotNull());
        scene_observer->scene_changed();
    }

    void damage_area(geom::Rectangle const& damage)
    {
        auto const locked = surface_observer.lock();
        ASSERT_THAT(locked, NotNull());
        locked->frame_posted(&surface, damage);
    }
};
}

TEST_F(DamageTrackerV1, does_not_register_scene_observer_when_not_used)
{
    EXPECT_CALL(surface_stack, add_observer(_)).Times(0);
    mf::WlrScreencopyV1DamageTracker tracker{executor, surface_stack};
}

TEST_F(DamageTrackerV1, captures_first_frame_immediately_when_requested)
{
    geom::Rectangle const area{{}, {20, 30}};
    StrictMock<MockFrame> frame{area};
    EXPECT_CALL(frame, capture(Eq(geom::Rectangle{{}, {20, 30}})));
    tracker.capture_on_damage(&frame);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_scene_changed_second_frame_is_captured)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    StrictMock<MockFrame> frame{area};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(geom::Rectangle{{}, {20, 30}})));
    damage_whole_scene();
    executor.execute();
}

TEST_F(DamageTrackerV1, when_area_damaged_second_frame_is_captured)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    StrictMock<MockFrame> frame{area};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(area)));
    damage_area(area);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_area_inside_capture_area_is_damaged_frame_is_captured_with_correct_damage)
{
    geom::Rectangle const capture_area{{}, {20, 30}};
    geom::Rectangle const area_inside_capture_area{{2, 4}, {5, 7}};
    capture_frame(capture_area, capture_area.size);

    StrictMock<MockFrame> frame{capture_area};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(area_inside_capture_area)));
    damage_area(area_inside_capture_area);
    executor.execute();
}

TEST_F(DamageTrackerV1, damage_clipped_to_capture_area)
{
    geom::Rectangle const capture_area{{}, {20, 30}};
    geom::Rectangle const area_intersecting_with_capture_area{{-10, 5}, {45, 66}};
    geom::Rectangle const expected_damage_area{{0, 5}, {20, 25}};
    capture_frame(capture_area, capture_area.size);

    StrictMock<MockFrame> frame{capture_area};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(expected_damage_area)));
    damage_area(area_intersecting_with_capture_area);
    executor.execute();
}

TEST_F(DamageTrackerV1, damage_is_relative_to_capture_area_position)
{
    geom::Rectangle const capture_area{{5, 12}, {20, 30}};
    geom::Rectangle const area_inside_capture_area{{10, 14}, {5, 5}};
    geom::Rectangle const expected_damage_area{{5, 2}, {5, 5}};
    capture_frame(capture_area, capture_area.size);

    StrictMock<MockFrame> frame{capture_area};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(expected_damage_area)));
    damage_area(area_inside_capture_area);
    executor.execute();
}

TEST_F(DamageTrackerV1, full_damage_is_scaled_to_buffer_size)
{
    geom::Rectangle const capture_area{{}, {20, 30}};
    geom::Size const buffer_size{40, 60};
    geom::Rectangle const expected_damage_area{{}, buffer_size};
    capture_frame(capture_area, buffer_size);

    StrictMock<MockFrame> frame{capture_area, buffer_size};
    EXPECT_CALL(frame, capture(Eq(expected_damage_area)));

    tracker.capture_on_damage(&frame);
    damage_whole_scene();
    executor.execute();
}

TEST_F(DamageTrackerV1, partial_damage_is_scaled_to_buffer_size)
{
    geom::Rectangle const capture_area{{}, {20, 30}};
    geom::Rectangle const area_of_damage{{}, {10, 10}};
    geom::Size const buffer_size{40, 60};
    geom::Rectangle const expected_buffer_damage{{}, {20, 20}};
    capture_frame(capture_area, buffer_size);

    StrictMock<MockFrame> frame{capture_area, buffer_size};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(expected_buffer_damage)));
    damage_area(area_of_damage);
    executor.execute();
}

TEST_F(DamageTrackerV1, damage_position_offset_is_scaled_to_buffer_size)
{
    geom::Rectangle const capture_area{{4, 6}, {20, 30}};
    geom::Rectangle const area_of_damage{{8, 20}, {10, 10}};
    geom::Size const buffer_size{40, 60};
    geom::Rectangle const expected_buffer_damage{{8, 28}, {20, 20}};
    capture_frame(capture_area, buffer_size);

    StrictMock<MockFrame> frame{capture_area, buffer_size};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(expected_buffer_damage)));
    damage_area(area_of_damage);
    executor.execute();
}

TEST_F(DamageTrackerV1, damage_size_can_be_scaled_fractionally)
{
    geom::Rectangle const capture_area{{}, {20, 30}};
    geom::Rectangle const area_of_damage{{4, 12}, {10, 10}};
    geom::Size const buffer_size{30, 45};
    geom::Rectangle const expected_buffer_damage{{6, 18}, {15, 15}};
    capture_frame(capture_area, buffer_size);

    StrictMock<MockFrame> frame{capture_area, buffer_size};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(expected_buffer_damage)));
    damage_area(area_of_damage);
    executor.execute();
}

TEST_F(DamageTrackerV1, damage_size_x_and_y_can_be_scaled_independently)
{
    geom::Rectangle const capture_area{{}, {20, 30}};
    geom::Rectangle const area_of_damage{{4, 12}, {10, 10}};
    geom::Size const buffer_size{30, 15};
    geom::Rectangle const expected_buffer_damage{{6, 6}, {15, 5}};
    capture_frame(capture_area, buffer_size);

    StrictMock<MockFrame> frame{capture_area, buffer_size};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(expected_buffer_damage)));
    damage_area(area_of_damage);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_damage_is_outside_of_frames_area_frame_is_not_captured)
{
    geom::Rectangle const area{{}, {20, 30}};
    geom::Rectangle const damage{{25, 0}, {5, 5}};
    capture_frame(area, area.size);

    StrictMock<MockFrame> frame{area};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(_)).Times(0);
    damage_area(damage);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_scene_has_changed_since_first_frame_the_second_frame_is_captured_immediately)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    damage_whole_scene();
    executor.execute();

    StrictMock<MockFrame> frame{area};
    EXPECT_CALL(frame, capture(_)).Times(1);
    tracker.capture_on_damage(&frame);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_area_has_been_damaged_since_first_frame_the_second_frame_is_captured_immediately)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    damage_area(area);
    executor.execute();

    StrictMock<MockFrame> frame{area};
    EXPECT_CALL(frame, capture(_)).Times(1);
    tracker.capture_on_damage(&frame);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_second_frame_is_captured_immediately_the_third_frame_is_not_captured_until_there_is_damage)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    damage_whole_scene();
    capture_frame(area, area.size);

    StrictMock<MockFrame> frame{area};
    tracker.capture_on_damage(&frame);
    executor.execute();

    EXPECT_CALL(frame, capture(Eq(geom::Rectangle{{}, {20, 30}})));
    damage_whole_scene();
    executor.execute();
}

TEST_F(DamageTrackerV1, captures_third_frame_immediately_after_getting_damage)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    damage_whole_scene();
    capture_frame(area, area.size);
    damage_whole_scene();

    StrictMock<MockFrame> frame{area};
    EXPECT_CALL(frame, capture(Eq(geom::Rectangle{{}, {20, 30}})));
    tracker.capture_on_damage(&frame);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_there_are_multiple_frames_and_only_one_is_damaged_the_correct_frame_is_captured)
{
    geom::Rectangle const area_a{{ 0, 0}, {20, 30}};
    geom::Rectangle const area_b{{25, 0}, {20, 30}};
    geom::Rectangle const area_c{{50, 0}, {20, 30}};
    capture_frame(area_a, area_a.size);
    capture_frame(area_b, area_a.size);
    capture_frame(area_c, area_a.size);

    StrictMock<MockFrame> frame_a{area_a};
    EXPECT_CALL(frame_a, capture(_)).Times(0);
    tracker.capture_on_damage(&frame_a);

    StrictMock<MockFrame> frame_b{area_b};
    tracker.capture_on_damage(&frame_b);

    StrictMock<MockFrame> frame_c{area_c};
    EXPECT_CALL(frame_c, capture(_)).Times(0);
    tracker.capture_on_damage(&frame_c);

    executor.execute();

    EXPECT_CALL(frame_b, capture(_));
    damage_area(area_b);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_tracker_is_destroyed_pending_frames_area_captured)
{
    geom::Rectangle const area_a{{ 0, 0}, {20, 30}};
    geom::Rectangle const area_b{{25, 0}, {20, 30}};

    std::optional<mf::WlrScreencopyV1DamageTracker> tracker;
    tracker.emplace(executor, surface_stack);
    {
        NiceMock<MockFrame> frame_a{area_a};
        tracker->capture_on_damage(&frame_a);
        NiceMock<MockFrame> frame_b{area_b};
        tracker->capture_on_damage(&frame_b);
        executor.execute();
    }

    StrictMock<MockFrame> frame_a{area_a};
    tracker->capture_on_damage(&frame_a);
    StrictMock<MockFrame> frame_b{area_b};
    tracker->capture_on_damage(&frame_b);
    executor.execute();

    EXPECT_CALL(frame_a, capture(_));
    EXPECT_CALL(frame_b, capture(_));
    tracker.reset();
    executor.execute();
}

TEST_F(DamageTrackerV1, creating_new_areas_does_not_result_in_pending_frames_being_captured)
{
    // This catches problems that may occur when the area list is expanded

    std::vector<std::shared_ptr<StrictMock<MockFrame>>> frames;

    for (int i = 0; i < 20; i++)
    {
        geom::Rectangle area{{i * 10, 0}, {5, 5}};
        capture_frame(area, area.size);
        frames.push_back(std::make_shared<StrictMock<MockFrame>>(area));
        EXPECT_CALL(*frames.back(), capture(_)).Times(0);
        tracker.capture_on_damage(frames.back().get());
    }
}

TEST_F(DamageTrackerV1, when_additional_frame_is_queued_with_same_area_then_pending_frame_is_captured_immediately)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    StrictMock<MockFrame> frame_a{area};
    tracker.capture_on_damage(&frame_a);
    executor.execute();

    EXPECT_CALL(frame_a, capture(_));
    StrictMock<MockFrame> frame_b{area};
    tracker.capture_on_damage(&frame_b);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_outputs_are_different_allows_multiple_pending_frames_with_the_same_area)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);
    wl_resource* output_a{reinterpret_cast<wl_resource*>(1)};
    wl_resource* output_b{reinterpret_cast<wl_resource*>(2)};

    {
        NiceMock<MockFrame> frame_a{area};
        frame_a.params.output = output_a;
        tracker.capture_on_damage(&frame_a);

        NiceMock<MockFrame> frame_b{area};
        frame_b.params.output = output_b;
        tracker.capture_on_damage(&frame_b);

        executor.execute();
    }

    StrictMock<MockFrame> frame_a{area};
    frame_a.params.output = output_a;
    tracker.capture_on_damage(&frame_a);
    executor.execute();

    StrictMock<MockFrame> frame_b{area};
    frame_b.params.output = output_b;
    tracker.capture_on_damage(&frame_b);
    executor.execute();

    EXPECT_CALL(frame_a, capture(_));
    EXPECT_CALL(frame_b, capture(_));
    damage_whole_scene();
    executor.execute();
}

TEST_F(DamageTrackerV1, when_second_area_is_already_damaged_and_first_area_has_pending_frame_second_frame_is_captured_on_second_area_immediately)
{
    geom::Rectangle const area_a{{ 0, 0}, {20, 30}};
    geom::Rectangle const area_b{{25, 0}, {20, 30}};
    capture_frame(area_a, area_a.size);
    capture_frame(area_b, area_b.size);

    damage_area(area_b);
    executor.execute();

    StrictMock<MockFrame> frame_a{area_a};
    EXPECT_CALL(frame_a, capture(_)).Times(0);
    tracker.capture_on_damage(&frame_a);
    executor.execute();

    StrictMock<MockFrame> frame_b{area_b};
    EXPECT_CALL(frame_b, capture(_));
    tracker.capture_on_damage(&frame_b);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_partial_damage_is_followed_by_full_damage_frame_is_captured_with_full_damage)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    damage_area(area);
    executor.execute();

    damage_whole_scene();
    executor.execute();

    StrictMock<MockFrame> frame{area};
    EXPECT_CALL(frame, capture(Eq(geom::Rectangle{{}, {20, 30}})));
    tracker.capture_on_damage(&frame);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_full_damage_is_followed_by_partial_damage_frame_is_captured_with_full_damage)
{
    geom::Rectangle const area{{}, {20, 30}};
    capture_frame(area, area.size);

    damage_whole_scene();
    executor.execute();

    damage_area(area);
    executor.execute();

    StrictMock<MockFrame> frame{area};
    EXPECT_CALL(frame, capture(Eq(geom::Rectangle{{}, {20, 30}})));
    tracker.capture_on_damage(&frame);
    executor.execute();
}

TEST_F(DamageTrackerV1, when_multiple_damages_happen_before_capture_frame_is_captured_with_combined_damage_areas)
{
    geom::Rectangle const area{{}, {20, 30}};
    geom::Rectangle const damage_a{{0, 5}, {10, 6}};
    geom::Rectangle const damage_b{{5, 5}, {10, 8}};
    geom::Rectangle const damage_combined{{0, 5}, {15, 8}};
    capture_frame(area, area.size);

    damage_area(damage_a);
    executor.execute();

    damage_area(damage_b);
    executor.execute();

    StrictMock<MockFrame> frame{area};
    EXPECT_CALL(frame, capture(Eq(damage_combined)));
    tracker.capture_on_damage(&frame);
    executor.execute();
}
