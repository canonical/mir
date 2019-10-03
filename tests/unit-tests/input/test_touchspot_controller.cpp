/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/touchspot_controller.h"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/renderable.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_scene.h"
#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/stub_input_scene.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <assert.h>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;

namespace
{

struct MockBufferAllocator : public mg::GraphicBufferAllocator
{
    MOCK_METHOD1(alloc_buffer, std::shared_ptr<mg::Buffer>(mg::BufferProperties const&));
    MOCK_METHOD0(supported_pixel_formats, std::vector<MirPixelFormat>(void));
    MOCK_METHOD2(alloc_software_buffer, std::shared_ptr<mg::Buffer>(geom::Size, MirPixelFormat));
    MOCK_METHOD3(alloc_buffer, std::shared_ptr<mg::Buffer>(geom::Size, uint32_t, uint32_t));
};

struct StubScene : public mtd::StubInputScene
{
    void add_input_visualization(std::shared_ptr<mg::Renderable> const& overlay) override
    {
        overlays.push_back(overlay);
    }

    void remove_input_visualization(std::weak_ptr<mg::Renderable> const& overlay) override
    {
        auto l = overlay.lock();
        assert(l);
        
        auto it = std::find(overlays.begin(), overlays.end(), l);
        assert(it != overlays.end());

        overlays.erase(it);
    }
    
    void expect_spots_centered_at(std::vector<geom::Point> spots)
    {
        int const touchspot_side_in_pixels = 64;
        for (auto overlay : overlays)
        {
            auto top_left_pos = overlay->screen_position().top_left;
            auto center_pos = geom::Point{top_left_pos.x.as_int() + touchspot_side_in_pixels/2,
                top_left_pos.y.as_int() + touchspot_side_in_pixels/2};
            auto it = std::find(spots.begin(), spots.end(), center_pos);
            EXPECT_FALSE(it == spots.end());
            spots.erase(it);
        }
        // If there are left over spots then we didn't have an overlay corresponding to one
        EXPECT_EQ(0, spots.size());
    }
    
    std::vector<std::shared_ptr<mg::Renderable>> overlays;
};

struct TestTouchspotController : public ::testing::Test
{
    TestTouchspotController()
        : allocator(std::make_shared<MockBufferAllocator>()),
          scene(std::make_shared<StubScene>())
    {
    }
    std::shared_ptr<MockBufferAllocator> const allocator;
    std::shared_ptr<StubScene> const scene;
};

}

TEST_F(TestTouchspotController, allocates_software_buffer_for_touchspots)
{
    using namespace ::testing;

    EXPECT_CALL(*allocator, alloc_software_buffer(_, _)).Times(1)
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));
    mi::TouchspotController controller(allocator, scene);
}

TEST_F(TestTouchspotController, touches_result_in_renderables_in_stack)
{
    using namespace ::testing;

    EXPECT_CALL(*allocator, alloc_software_buffer(_, _)).Times(1)
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));
    mi::TouchspotController controller(allocator, scene);
    controller.enable();
    
    controller.visualize_touches({ {{0,0}, 1} });

    scene->expect_spots_centered_at({{0, 0}});
}

TEST_F(TestTouchspotController, spots_move)
{
    using namespace ::testing;
    
    EXPECT_CALL(*allocator, alloc_software_buffer(_, _)).Times(1)
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));
    mi::TouchspotController controller(allocator, scene);
    controller.enable();
    
    controller.visualize_touches({ {{0,0}, 1} });
    scene->expect_spots_centered_at({{0, 0}});
    controller.visualize_touches({ {{1,1}, 1} });
    scene->expect_spots_centered_at({{1, 1}});
}

TEST_F(TestTouchspotController, multiple_spots)
{
    using namespace ::testing;
    
    EXPECT_CALL(*allocator, alloc_software_buffer(_, _)).Times(1)
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));
    mi::TouchspotController controller(allocator, scene);
    controller.enable();
    
    controller.visualize_touches({ {{0,0}, 1}, {{1, 1}, 1}, {{3, 3}, 1} });
    scene->expect_spots_centered_at({{0, 0}, {1, 1}, {3, 3}});
    controller.visualize_touches({ {{0,0}, 1}, {{1, 1}, 1}, {{3, 3}, 1}, {{5, 5}, 1} });
    scene->expect_spots_centered_at({{0, 0}, {1, 1}, {3, 3}, {5, 5}});
    controller.visualize_touches({ {{1,1}, 1} });
    scene->expect_spots_centered_at({{1, 1}});
    controller.visualize_touches({});
    scene->expect_spots_centered_at({});
}

// This leaves some semantics undefined, i,e. if the touchspot controller is enabled/disabled
// during a gesture do the spots appear/dissapear? I've been unable to develop a strong opinion
// on this semantic, so I am leaving it unspecified ~racarr
TEST_F(TestTouchspotController, touches_do_not_result_in_renderables_in_stack_when_disabled)
{
    using namespace ::testing;
    
    EXPECT_CALL(*allocator, alloc_software_buffer(_, _)).Times(1)
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));
    mi::TouchspotController controller(allocator, scene);
    controller.enable();

    controller.disable();
    controller.visualize_touches({ {{0,0}, 1} });

    scene->expect_spots_centered_at({});

    controller.enable();
    controller.visualize_touches({ {{0,0}, 1} });

    scene->expect_spots_centered_at({{0, 0}});
}

namespace
{

struct StubSceneWithMockEmission : public StubScene
{
    MOCK_METHOD0(emit_scene_changed, void());
};

struct TestTouchspotControllerSceneUpdates : public TestTouchspotController
{
    TestTouchspotControllerSceneUpdates()
        : scene(std::make_shared<StubSceneWithMockEmission>())
    {
        using namespace ::testing;
        EXPECT_CALL(*allocator, alloc_software_buffer(_, _)).Times(1)
            .WillOnce(testing::Return(std::make_shared<mtd::StubBuffer>()));
    }

    std::shared_ptr<StubSceneWithMockEmission> const scene;
};

}

TEST_F(TestTouchspotControllerSceneUpdates, does_not_emit_damage_if_nothing_happens)
{
    EXPECT_CALL(*scene, emit_scene_changed()).Times(0);

    mi::TouchspotController controller(allocator, scene);

    // We are disabled so no damage should occur.
    controller.visualize_touches({ {{0,0}, 1} });

    controller.enable();
    // Now we are enabled but do nothing so still no damage should occur.
    controller.visualize_touches({});
}

TEST_F(TestTouchspotControllerSceneUpdates, emits_scene_damage)
{
    EXPECT_CALL(*scene, emit_scene_changed()).Times(2);

    mi::TouchspotController controller(allocator, scene);

    controller.enable();
    controller.visualize_touches({ {{0,0}, 1} });
    controller.visualize_touches({ {{1,1}, 1}});
}
