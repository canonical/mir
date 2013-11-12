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

#include "src/server/graphics/android/hwc_layerlist.h"
#include "mir_test_doubles/mock_buffer.h"
#include "hwc_struct_helper-inl.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

class HWCLayerListTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        width = 432; 
        height = 876; 
        default_size = geom::Size{width, height};

        native_handle_1 = std::make_shared<mtd::StubAndroidNativeBuffer>();
        native_handle_2 = std::make_shared<mtd::StubAndroidNativeBuffer>();

        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>();
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_handle_1));
        ON_CALL(*mock_buffer, size())
            .WillByDefault(Return(default_size));
    }

    int width; 
    int height; 
    geom::Size default_size;

    std::shared_ptr<mg::NativeBuffer> native_handle_1;
    std::shared_ptr<mg::NativeBuffer> native_handle_2;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
};

TEST(HWCLayerDeepCopy, hwc_layer)
{
    mga::HWCDefaultLayer original({});
    hwc_layer_1 layer = original;
    EXPECT_EQ(0u, layer.visibleRegionScreen.numRects); 
    EXPECT_EQ(nullptr, layer.visibleRegionScreen.rects);

    geom::Rectangle r0{geom::Point{geom::X{0},geom::Y{1}},
                       geom::Size{2, 3}};
    geom::Rectangle r1{geom::Point{geom::X{0},geom::Y{1}},
                       geom::Size{3, 3}};
    geom::Rectangle r2{geom::Point{geom::X{1},geom::Y{1}},
                       geom::Size{2, 3}};
    mga::HWCRect a(r0), b(r1), c(r2);
    mga::HWCDefaultLayer original2({a, b, c});
    layer = original2;

    ASSERT_EQ(3u, layer.visibleRegionScreen.numRects);
    ASSERT_NE(nullptr, layer.visibleRegionScreen.rects);
    EXPECT_THAT(a, HWCRectMatchesRect(layer.visibleRegionScreen.rects[0],""));
    EXPECT_THAT(b, HWCRectMatchesRect(layer.visibleRegionScreen.rects[1],""));
    EXPECT_THAT(c, HWCRectMatchesRect(layer.visibleRegionScreen.rects[2],""));
}

TEST_F(HWCLayerListTest, hwc_list_creation_loads_latest_fb_target)
{
    using namespace testing;
   
    hwc_rect_t expected_sc, expected_df, expected_visible;
    expected_sc = {0, 0, width, height};
    expected_df = expected_visible = expected_sc;
    EXPECT_CALL(*mock_buffer, size())
        .Times(1)
        .WillOnce(Return(default_size));

    mga::LayerList layerlist;
    layerlist.set_fb_target(mock_buffer);

    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list->numHwLayers);
    hwc_layer_1 target_layer = list->hwLayers[0];
    EXPECT_THAT(target_layer.sourceCrop, MatchesRect( expected_sc, "sourceCrop"));
    EXPECT_THAT(target_layer.displayFrame, MatchesRect( expected_df, "displayFrame"));

    ASSERT_EQ(1u, target_layer.visibleRegionScreen.numRects); 
    ASSERT_NE(nullptr, target_layer.visibleRegionScreen.rects); 
    EXPECT_THAT(target_layer.visibleRegionScreen.rects[0], MatchesRect( expected_visible, "visible"));
}

TEST_F(HWCLayerListTest, fb_target)
{
    using namespace testing;

    mga::LayerList layerlist;

    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list->numHwLayers);
    hwc_layer_1 target_layer = list->hwLayers[0];
    EXPECT_EQ(nullptr, target_layer.handle); 
}

TEST_F(HWCLayerListTest, set_fb_target_gets_fb_handle)
{
    using namespace testing;

    mga::LayerList layerlist;

    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(native_handle_1));

    layerlist.set_fb_target(mock_buffer);
    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list->numHwLayers);
    hwc_layer_1 target_layer = list->hwLayers[0]; 
    EXPECT_EQ(native_handle_1->handle(), target_layer.handle); 
}

TEST_F(HWCLayerListTest, set_fb_target_2x)
{
    using namespace testing;

    mga::LayerList layerlist;

    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .Times(2)
        .WillOnce(Return(native_handle_1))
        .WillOnce(Return(native_handle_2));

    layerlist.set_fb_target(mock_buffer);
    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list->numHwLayers);
    hwc_layer_1 target_layer = list->hwLayers[0]; 
    EXPECT_EQ(native_handle_1->handle(), target_layer.handle); 

    layerlist.set_fb_target(mock_buffer);
    auto list_second = layerlist.native_list();
    ASSERT_EQ(1u, list_second->numHwLayers);
    target_layer = list_second->hwLayers[0]; 
    EXPECT_EQ(native_handle_2->handle(), target_layer.handle); 
}

TEST_F(HWCLayerListTest, set_fb_target_programs_other_struct_members_correctly)
{
    using namespace testing;

    mga::LayerList layerlist;
    layerlist.set_fb_target(mock_buffer);

    hwc_rect_t source_region = {0,0,width, height};
    hwc_rect_t target_region = source_region;
    hwc_region_t region {1, nullptr};

    hwc_layer_1 expected_layer;
    expected_layer.compositionType = HWC_FRAMEBUFFER_TARGET;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_NONE;
    expected_layer.sourceCrop = source_region;
    expected_layer.displayFrame = target_region; 
    expected_layer.visibleRegionScreen = region;  
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list->numHwLayers);
    hwc_layer_1 target_layer = list->hwLayers[0]; 
    EXPECT_THAT(target_layer, MatchesLayer( expected_layer ));
}
