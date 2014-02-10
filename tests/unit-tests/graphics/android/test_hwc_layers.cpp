/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/platform/graphics/android/hwc_layerlist.h"
#include "mir_test_doubles/mock_buffer.h"
#include "hwc_struct_helpers.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test_doubles/mock_renderable.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

class HWCLayersTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        native_handle_1 = std::make_shared<mtd::StubAndroidNativeBuffer>();

        ON_CALL(mock_buffer, size())
            .WillByDefault(Return(buffer_size));
        ON_CALL(mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_handle_1));

        list = std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>(
                ::operator new(sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t))));
        list_index = 0;
        hwc_layer = &list->hwLayers[list_index];
        type = mga::LayerType::gl_rendered;
    }

    mga::LayerType type;
    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},{245, 250}};
    bool alpha_enabled{false};
    std::shared_ptr<mtd::StubAndroidNativeBuffer> native_handle_1;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;

    std::shared_ptr<hwc_display_contents_1_t> list;
    hwc_layer_1_t* hwc_layer;
    size_t list_index;
};

TEST_F(HWCLayersTest, checking_release_fences)
{
    int fence1 = 1, fence2 = -1;
    mga::HWCLayer layer(list, list_index);

    hwc_layer->releaseFenceFd = fence1;
    EXPECT_EQ(layer.release_fence(), fence1);

    hwc_layer->releaseFenceFd = fence2;
    EXPECT_EQ(layer.release_fence(), fence2);
}

TEST_F(HWCLayersTest, check_if_layer_needs_gl_render)
{
    mga::HWCLayer layer(list, list_index);

    hwc_layer->compositionType = HWC_OVERLAY;
    EXPECT_FALSE(layer.needs_gl_render());

    hwc_layer->flags = HWC_SKIP_LAYER;
    EXPECT_TRUE(layer.needs_gl_render());

    hwc_layer->flags = 0;
    hwc_layer->compositionType = HWC_FRAMEBUFFER;
    EXPECT_TRUE(layer.needs_gl_render());
}

TEST_F(HWCLayersTest, move_layer_positions)
{
    hwc_rect_t region = {0,0,0,0};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;
    memset(&expected_layer, 0, sizeof(expected_layer));
    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = nullptr;
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_NONE;
    expected_layer.sourceCrop = region;
    expected_layer.displayFrame = {
        screen_position.top_left.x.as_int(),
        screen_position.top_left.y.as_int(),
        screen_position.size.width.as_int(),
        screen_position.size.height.as_int()};
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    mga::HWCLayer layer(type, screen_position, false, list, list_index);
    mga::HWCLayer second_layer(std::move(layer));

    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayersTest, change_layer_types)
{
    hwc_rect_t region = {0,0,0,0};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;
    memset(&expected_layer, 0, sizeof(expected_layer));
    expected_layer.compositionType = HWC_FRAMEBUFFER_TARGET;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = nullptr;
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_NONE;
    expected_layer.sourceCrop = region;
    expected_layer.displayFrame = {
        screen_position.top_left.x.as_int(),
        screen_position.top_left.y.as_int(),
        screen_position.size.width.as_int(),
        screen_position.size.height.as_int()};
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    type = mga::LayerType::framebuffer_target;
    mga::HWCLayer layer(type, screen_position, alpha_enabled, list, list_index);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));

    EXPECT_THROW({
        layer.set_layer_type(mga::LayerType::overlay);
    }, std::logic_error);

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    layer.set_layer_type(mga::LayerType::gl_rendered);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.flags = HWC_SKIP_LAYER;
    layer.set_layer_type(mga::LayerType::skip);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.flags = 0;
    layer.set_layer_type(mga::LayerType::gl_rendered);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayersTest, apply_buffer_updates_to_layers)
{
    int fake_fence = 552;
    ON_CALL(*native_handle_1, copy_fence())
        .WillByDefault(testing::Return(fake_fence));

    hwc_rect_t region = {0,0,buffer_size.width.as_int(), buffer_size.height.as_int()};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;
    memset(&expected_layer, 0, sizeof(expected_layer));
    expected_layer.compositionType = HWC_FRAMEBUFFER_TARGET;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_NONE;
    expected_layer.sourceCrop = region;
    expected_layer.displayFrame = {
        screen_position.top_left.x.as_int(),
        screen_position.top_left.y.as_int(),
        screen_position.size.width.as_int(),
        screen_position.size.height.as_int()};
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = fake_fence;
    expected_layer.releaseFenceFd = -1;

    type = mga::LayerType::framebuffer_target;

    mga::HWCLayer layer(type, screen_position, alpha_enabled, list, list_index);

    //must reset this to -1
    hwc_layer->releaseFenceFd = fake_fence;
    layer.set_buffer(mt::fake_shared(mock_buffer));
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayersTest, buffer_fence_update)
{
    int fake_fence = 552;
    EXPECT_CALL(*native_handle_1, update_fence(fake_fence))
        .Times(1);
    type = mga::LayerType::framebuffer_target;
    mga::HWCLayer layer(type, screen_position, alpha_enabled, list, list_index);

    layer.set_buffer(mt::fake_shared(mock_buffer));
    hwc_layer->releaseFenceFd = fake_fence;
    layer.update_buffer_fence();

    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}


TEST_F(HWCLayersTest, check_layer_defaults_and_alpha)
{
    using namespace testing;

    hwc_rect_t crop
    {
        0,0,
        buffer_size.width.as_int(),
        buffer_size.height.as_int()
    };

    hwc_rect_t screen_pos
    {
        screen_position.top_left.x.as_int(),
        screen_position.top_left.y.as_int(),
        screen_position.size.width.as_int(),
        screen_position.size.height.as_int()
    };

    hwc_region_t visible_region {1, &screen_pos};
    hwc_layer_1 expected_layer;

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_COVERAGE;
    expected_layer.sourceCrop = crop;
    expected_layer.displayFrame = screen_pos;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    mga::HWCLayer layer(list, list_index);
    layer.set_render_parameters(screen_position, true);
    layer.set_buffer(mt::fake_shared(mock_buffer));
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));

    expected_layer.blending = HWC_BLENDING_NONE;
    layer.set_render_parameters(screen_position, false);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}
