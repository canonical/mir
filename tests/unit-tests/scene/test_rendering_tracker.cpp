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

#include "src/server/scene/rendering_tracker.h"
#include "mir/test/doubles/mock_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;

namespace
{

struct RenderingTrackerTest : testing::Test
{
    std::shared_ptr<testing::NiceMock<mtd::MockSurface>> const mock_surface{
        std::make_shared<testing::NiceMock<mtd::MockSurface>>()};
    mir::scene::RenderingTracker tracker{mock_surface};
    mc::CompositorID const compositor_id1{&mock_surface};
    mc::CompositorID const compositor_id2{&compositor_id1};
    mc::CompositorID const compositor_id3{&compositor_id2};
};

}

TEST_F(RenderingTrackerTest, occludes_surface_when_occluded_in_single_compositor)
{
    using namespace testing;

    std::set<mc::CompositorID> const compositors{compositor_id1};

    EXPECT_CALL(
        *mock_surface,
        configure(mir_surface_attrib_visibility, mir_surface_visibility_occluded));

    tracker.active_compositors(compositors);

    tracker.occluded_in(compositor_id1);
}

TEST_F(RenderingTrackerTest, exposes_surface_when_rendered_in_single_compositor)
{
    using namespace testing;

    std::set<mc::CompositorID> const compositors{compositor_id1};

    EXPECT_CALL(
        *mock_surface,
        configure(mir_surface_attrib_visibility, mir_surface_visibility_exposed));

    tracker.active_compositors(compositors);

    tracker.rendered_in(compositor_id1);
}

TEST_F(RenderingTrackerTest, exposes_surface_when_rendered_in_one_of_many_compositors)
{
    using namespace testing;

    std::set<mc::CompositorID> const compositors{compositor_id1, compositor_id2, compositor_id3};

    EXPECT_CALL(
        *mock_surface,
        configure(mir_surface_attrib_visibility, mir_surface_visibility_exposed));

    tracker.active_compositors(compositors);

    tracker.occluded_in(compositor_id1);
    tracker.rendered_in(compositor_id2);
}

TEST_F(RenderingTrackerTest, does_not_occlude_surface_when_not_occluded_in_all_compositors)
{
    using namespace testing;

    std::set<mc::CompositorID> const compositors{compositor_id1, compositor_id2, compositor_id3};

    EXPECT_CALL(
        *mock_surface,
        configure(mir_surface_attrib_visibility, mir_surface_visibility_occluded))
        .Times(0);

    tracker.active_compositors(compositors);

    tracker.occluded_in(compositor_id1);
    tracker.occluded_in(compositor_id2);
}

TEST_F(RenderingTrackerTest, occludes_surface_when_occluded_in_all_compositors)
{
    using namespace testing;

    std::set<mc::CompositorID> const compositors{compositor_id1, compositor_id2, compositor_id3};

    EXPECT_CALL(
        *mock_surface,
        configure(mir_surface_attrib_visibility, mir_surface_visibility_occluded));

    tracker.active_compositors(compositors);

    tracker.occluded_in(compositor_id1);
    tracker.occluded_in(compositor_id2);
    tracker.occluded_in(compositor_id3);
}

TEST_F(RenderingTrackerTest, occludes_surface_when_occluded_in_remaining_compositors_after_removing_compositor)
{
    using namespace testing;

    std::set<mc::CompositorID> compositors{compositor_id1, compositor_id2, compositor_id3};

    tracker.active_compositors(compositors);

    tracker.occluded_in(compositor_id1);
    tracker.occluded_in(compositor_id2);
    tracker.rendered_in(compositor_id3);

    Mock::VerifyAndClearExpectations(mock_surface.get());

    EXPECT_CALL(
        *mock_surface,
        configure(mir_surface_attrib_visibility, mir_surface_visibility_occluded));

    compositors.erase(compositor_id3);
    tracker.active_compositors(compositors);
}

TEST_F(RenderingTrackerTest, throws_when_passed_inactive_compositor_id)
{
    EXPECT_THROW({
        tracker.occluded_in(compositor_id1);
    }, std::logic_error);

    EXPECT_THROW({
        tracker.rendered_in(compositor_id2);
    }, std::logic_error);
}
