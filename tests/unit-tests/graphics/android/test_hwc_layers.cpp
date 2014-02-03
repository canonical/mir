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
#include "hwc_struct_helper-inl.h"
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
        native_handle_1->anwb()->width = width;
        native_handle_1->anwb()->height = height;
        native_handle_2 = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();

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
    bool alpha_enabled{true};
    int width{432};
    int height{876};
    std::shared_ptr<mg::NativeBuffer> native_handle_1;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> native_handle_2;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;

    std::shared_ptr<hwc_display_contents_1_t> list;
    hwc_layer_1_t* hwc_layer;
    size_t list_index;
};

TEST_F(HWCLayersTest, release_fences)
{
    int fence1 = 1, fence2 = -1;
    mga::HWCLayer layer(list, list_index);

    hwc_layer->releaseFenceFd = fence1;
    EXPECT_EQ(layer.release_fence(), fence1);

    hwc_layer->releaseFenceFd = fence2;
    EXPECT_EQ(layer.release_fence(), fence2);
}

TEST_F(HWCLayersTest, needs_gl_render)
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

TEST_F(HWCLayersTest, layer_move)
{
    hwc_rect_t region = {0,0,width, height};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;
    memset(&expected_layer, 0, sizeof(expected_layer));
    hwc_layer->compositionType = HWC_FRAMEBUFFER_TARGET;
    hwc_layer->hints = 0;
    hwc_layer->flags = 0;
    hwc_layer->handle = native_handle_1->handle();
    hwc_layer->transform = 0;
    hwc_layer->blending = HWC_BLENDING_NONE;
    hwc_layer->sourceCrop = region;
    hwc_layer->displayFrame = region;
    hwc_layer->visibleRegionScreen = visible_region;
    hwc_layer->acquireFenceFd = -1;
    hwc_layer->releaseFenceFd = -1;

    mga::HWCLayer layer(list, list_index);
    mga::HWCLayer second_layer(std::move(layer));

    second_layer.set_layer_type(mga::LayerType::framebuffer_target);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayersTest, layer_types)
{
    hwc_rect_t region = {0,0,width, height};
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
    expected_layer.displayFrame = region;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    type = mga::LayerType::framebuffer_target;
    mga::HWCLayer layer(type, screen_position, alpha_enabled, list, list_index);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));

    type = mga::LayerType::overlay;
    EXPECT_THROW({
        mga::HWCLayer layer(type, screen_position, alpha_enabled, list, list_index);
    }, std::logic_error);

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    type = mga::LayerType::gl_rendered;
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.flags = HWC_SKIP_LAYER;
    type = mga::LayerType::skip;
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayersTest, buffer_updates)
{
    hwc_rect_t region = {0,0,width, height};
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
    expected_layer.displayFrame = region;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    type = mga::LayerType::framebuffer_target;
    mga::HWCLayer layer(type, screen_position, alpha_enabled, list, list_index);

    layer.set_buffer(*native_handle_1);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayersTest, normal_layer)
{
    using namespace testing;

    hwc_rect_t crop
    {
        0,0,
        static_cast<int>(buffer_size.width.as_uint32_t()),
        static_cast<int>(buffer_size.height.as_uint32_t())
    };

    hwc_rect_t screen_pos
    {
        static_cast<int>(screen_position.top_left.x.as_uint32_t()),
        static_cast<int>(screen_position.top_left.y.as_uint32_t()),
        static_cast<int>(screen_position.size.width.as_uint32_t()),
        static_cast<int>(screen_position.size.height.as_uint32_t())
    };

    hwc_region_t visible_region {1, &screen_pos};
    hwc_layer_1 expected_layer;

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = 0;
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_COVERAGE;
    expected_layer.sourceCrop = crop;
    expected_layer.displayFrame = screen_pos;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    mga::HWCLayer layer(list, list_index);
    layer.set_render_parameters(screen_position, true);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));

    expected_layer.blending = HWC_BLENDING_NONE;
    layer.set_render_parameters(screen_position, false);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}
