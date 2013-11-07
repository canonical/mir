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
        native_handle_1 = std::make_shared<mtd::StubAndroidNativeBuffer>();
        native_handle_1->anwb()->width = width;
        native_handle_1->anwb()->height = height;
        native_handle_2 = std::make_shared<mtd::StubAndroidNativeBuffer>();
    }

    int width; 
    int height; 
    std::shared_ptr<mg::NativeBuffer> native_handle_1;
    std::shared_ptr<mg::NativeBuffer> native_handle_2;
};

TEST_F(HWCLayerListTest, fb_target_layer)
{
    mga::FramebufferLayer target_layer(*native_handle_1);
    EXPECT_EQ(mga::HWCLayerType::framebuffer, target_layer.type());

    hwc_rect_t region = {0,0,width, height};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;
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

    EXPECT_THAT(target_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayerListTest, gl_target_layer_with_force_gl)
{
    bool force_gl = true;
    mga::CompositionLayer target_layer(*native_handle_1, force_gl);
    EXPECT_EQ(mga::HWCLayerType::gles, target_layer.type());

    hwc_rect_t region = {0,0,width, height};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;
    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = HWC_SKIP_LAYER;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_NONE;
    expected_layer.sourceCrop = region;
    expected_layer.displayFrame = region; 
    expected_layer.visibleRegionScreen = visible_region;  
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    EXPECT_THAT(target_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayerListTest, gl_target_layer_without_force_gl)
{
    bool force_gl = false;
    mga::CompositionLayer target_layer(*native_handle_1, force_gl);
    EXPECT_EQ(mga::HWCLayerType::gles, target_layer.type());

    hwc_rect_t region = {0,0,width, height};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;
    expected_layer.compositionType = HWC_FRAMEBUFFER;
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

    EXPECT_THAT(target_layer, MatchesLayer(expected_layer));
}

//prepare() from hwcomposer.h is allowed to mutate the HWC_FRAMEBUFFER type like this
TEST_F(HWCLayerListTest, gl_target_layer_mutation_to_overlay)
{
    bool force_gl = true;
    mga::CompositionLayer target_layer(*native_handle_1, force_gl);
    EXPECT_EQ(mga::HWCLayerType::gles, target_layer.type());
 
    target_layer.compositionType = HWC_OVERLAY;

    EXPECT_EQ(mga::HWCLayerType::overlay, target_layer.type());
}

#if 0
TEST_F(HWCLayerListTest, hwc_list_creation)
{
    using namespace testing;
    mga::FramebufferLayer target_layer(mock_buffer);
    mga::CompositionLayer surface_layer(mock_buffer, force_gl);
    hwc_layer_1* target_layer_ptr = &target_layer;
    hwc_layer_1* surface_layer_ptr = &surface_layer;
   
    mga::LayerList layerlist({surface_layer, target_layer});

    auto list = layerlist.native_list(); 
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags); 
    /* note, mali hwc1.1 actually falsely returns if these are not set to something. set to garbage */ 
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);

    ASSERT_EQ(2u, list->numHwLayers);
    EXPECT_EQ(surface_layer, list->hwLayers[0]);
    EXPECT_EQ(target_layer, list->hwLayers[1]);
}
#endif
