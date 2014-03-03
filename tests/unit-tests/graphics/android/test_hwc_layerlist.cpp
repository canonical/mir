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
#include "src/platform/graphics/android/hwc_layers.h"
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

class HWCLayerListTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        native_handle_1.anwb()->width = width;
        native_handle_1.anwb()->height = height;

        ON_CALL(mock_buffer, size())
            .WillByDefault(Return(buffer_size));
        ON_CALL(mock_buffer, native_buffer_handle())
            .WillByDefault(Return(mt::fake_shared(native_handle_1)));
        ON_CALL(mock_renderable, buffer(_))
            .WillByDefault(Return(mt::fake_shared(mock_buffer)));
        ON_CALL(mock_renderable, alpha_enabled())
            .WillByDefault(Return(alpha_enabled));
        ON_CALL(mock_renderable, screen_position())
            .WillByDefault(Return(screen_position));

        empty_region = {0,0,0,0};
        set_region = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()};
        screen_pos = {
            screen_position.top_left.x.as_int(),
            screen_position.top_left.y.as_int(),
            screen_position.size.width.as_int(),
            screen_position.size.height.as_int()
        };

        comp_layer.compositionType = HWC_FRAMEBUFFER;
        comp_layer.hints = 0;
        comp_layer.flags = 0;
        comp_layer.handle = native_handle_1.handle();
        comp_layer.transform = 0;
        comp_layer.blending = HWC_BLENDING_COVERAGE;
        comp_layer.sourceCrop = set_region;
        comp_layer.displayFrame = screen_pos;
        comp_layer.visibleRegionScreen = {1, &set_region};
        comp_layer.acquireFenceFd = -1;
        comp_layer.releaseFenceFd = -1;

        target_layer.compositionType = HWC_FRAMEBUFFER_TARGET;
        target_layer.hints = 0;
        target_layer.flags = 0;
        target_layer.handle = 0;
        target_layer.transform = 0;
        target_layer.blending = HWC_BLENDING_NONE;
        target_layer.sourceCrop = empty_region;
        target_layer.displayFrame = empty_region;
        target_layer.visibleRegionScreen = {1, &set_region};
        target_layer.acquireFenceFd = -1;
        target_layer.releaseFenceFd = -1;

        skip_layer.compositionType = HWC_FRAMEBUFFER;
        skip_layer.hints = 0;
        skip_layer.flags = HWC_SKIP_LAYER;
        skip_layer.handle = 0;
        skip_layer.transform = 0;
        skip_layer.blending = HWC_BLENDING_NONE;
        skip_layer.sourceCrop = empty_region;
        skip_layer.displayFrame = empty_region;
        skip_layer.visibleRegionScreen = {1, &set_region};
        skip_layer.acquireFenceFd = -1;
        skip_layer.releaseFenceFd = -1;

        set_skip_layer = skip_layer;
        set_skip_layer.handle = native_handle_1.handle();
        set_skip_layer.sourceCrop = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()}; 
        set_skip_layer.displayFrame = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()};

        set_target_layer = target_layer;
        set_target_layer.handle = native_handle_1.handle();
        set_target_layer.sourceCrop = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()}; 
        set_target_layer.displayFrame = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()};
    }

    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},{245, 250}};
    bool alpha_enabled{true};
    int width{432};
    int height{876};
    mtd::StubAndroidNativeBuffer native_handle_1;
    testing::NiceMock<mtd::MockRenderable> mock_renderable;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;

    hwc_rect_t screen_pos;
    hwc_rect_t empty_region;
    hwc_rect_t set_region;
    hwc_layer_1_t skip_layer;
    hwc_layer_1_t target_layer;
    hwc_layer_1_t set_skip_layer;
    hwc_layer_1_t set_target_layer;
    hwc_layer_1_t comp_layer;
};

TEST_F(HWCLayerListTest, list_defaults)
{
    mga::LayerList layerlist;

    auto list = layerlist.native_list().lock();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
}

/* Tests without HWC_FRAMEBUFFER_TARGET */
/* an empty list should have a skipped layer. This will force a GL render on hwc set */
TEST_F(HWCLayerListTest, hwc10_list_defaults)
{
    mga::LayerList layerlist;

    auto list = layerlist.native_list().lock();
    ASSERT_EQ(1, list->numHwLayers);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
}

/* tests with a FRAMEBUFFER_TARGET present */
TEST_F(HWCLayerListTest, fbtarget_list_initialize)
{
    mga::FBTargetLayerList layerlist;
    auto list = layerlist.native_list().lock();
    ASSERT_EQ(2, list->numHwLayers);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fb_target_set)
{
    mga::FBTargetLayerList layerlist;

    layerlist.set_fb_target(mock_buffer);

    auto list = layerlist.native_list().lock();
    ASSERT_EQ(2, list->numHwLayers);
    EXPECT_THAT(set_skip_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(set_target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fbtarget_list_update)
{
    using namespace testing;
    mga::FBTargetLayerList layerlist;

    /* set non-default renderlist */
    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        mt::fake_shared(mock_renderable),
        mt::fake_shared(mock_renderable)
    });
    layerlist.set_composition_layers(updated_list);

    auto list = layerlist.native_list().lock();
    ASSERT_EQ(3, list->numHwLayers);
    EXPECT_THAT(comp_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(comp_layer, MatchesLayer(list->hwLayers[1]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[2]));

    /* update FB target */
    layerlist.set_fb_target(mock_buffer);

    list = layerlist.native_list().lock();
    target_layer.handle = native_handle_1.handle();
    ASSERT_EQ(3, list->numHwLayers);
    EXPECT_THAT(comp_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(comp_layer, MatchesLayer(list->hwLayers[1]));
    EXPECT_THAT(set_target_layer, MatchesLayer(list->hwLayers[2]));

    /* reset default */
    layerlist.reset_composition_layers();

    list = layerlist.native_list().lock();
    target_layer.handle = nullptr;
    ASSERT_EQ(2, list->numHwLayers);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, list_returns_fb_fence)
{
    int release_fence = 381;
    mga::FBTargetLayerList layerlist;

    auto list = layerlist.native_list().lock();
    ASSERT_EQ(2, list->numHwLayers);
    list->hwLayers[1].releaseFenceFd = release_fence;
    EXPECT_EQ(release_fence, layerlist.fb_target_fence());
}

TEST_F(HWCLayerListTest, list_returns_retire_fence)
{
    int release_fence = 381;
    mga::FBTargetLayerList layerlist;

    auto list = layerlist.native_list().lock();
    list->retireFenceFd = release_fence;
    EXPECT_EQ(release_fence, layerlist.retirement_fence());
}
