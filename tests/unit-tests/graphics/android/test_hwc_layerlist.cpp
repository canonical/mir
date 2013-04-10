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
#include "src/server/graphics/android/native_buffer_handle.h"
#include "mir_test_doubles/mock_buffer.h"
#include "hwc_struct_helper-inl.h"
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;
namespace mc=mir::compositor;

class HWCLayerListTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        width = 432; 
        height = 876; 
        default_size = geom::Size{geom::Width{width},
                       geom::Height{height}};

        stub_handle_1 = std::make_shared<mc::NativeBufferHandle>();
        stub_handle_2 = std::make_shared<mc::NativeBufferHandle>();

        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>();
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(stub_handle_1));
        ON_CALL(*mock_buffer, size())
            .WillByDefault(Return(default_size));

    }

    int width; 
    int height; 
    geom::Size default_size;

    std::shared_ptr<mc::NativeBufferHandle> stub_handle_1;
    std::shared_ptr<mc::NativeBufferHandle> stub_handle_2;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
};

TEST_F(HWCLayerListTest, default_list)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    auto list = layerlist.native_list(); 
    ASSERT_EQ(0u, list.size());
}

TEST_F(HWCLayerListTest, set_fb_target_figures_out_buffer_size)
{
    using namespace testing;
   
    hwc_rect_t expected_sc, expected_df, expected_visible;
    expected_sc = {0, 0, width, height};
    expected_df = expected_visible = expected_sc;
    EXPECT_CALL(*mock_buffer, size())
        .Times(1)
        .WillOnce(Return(default_size));

    mga::HWCLayerList layerlist;
    layerlist.set_fb_target(mock_buffer);

    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list.size());
    auto target_layer = list[0];
    EXPECT_THAT(target_layer->sourceCrop, MatchesRect( expected_sc, "sourceCrop"));
    EXPECT_THAT(target_layer->displayFrame, MatchesRect( expected_df, "displayFrame"));

    ASSERT_EQ(1u, target_layer->visibleRegionScreen.numRects); 
    ASSERT_NE(nullptr, target_layer->visibleRegionScreen.rects); 
    EXPECT_THAT(target_layer->visibleRegionScreen.rects[0], MatchesRect( expected_visible, "visible"));
}

TEST_F(HWCLayerListTest, set_fb_target_gets_fb_handle)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_handle_1));

    layerlist.set_fb_target(mock_buffer);
    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list.size());
    auto target_layer = list[0];
    EXPECT_EQ(stub_handle_1->handle, target_layer->handle); 
}

TEST_F(HWCLayerListTest, set_fb_target_2x)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .Times(2)
        .WillOnce(Return(stub_handle_1))
        .WillOnce(Return(stub_handle_2));

    layerlist.set_fb_target(mock_buffer);
    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list.size());
    auto target_layer = list[0];
    EXPECT_EQ(stub_handle_1->handle, list[0]->handle); 

    layerlist.set_fb_target(mock_buffer);
    auto list_second = layerlist.native_list();
    ASSERT_EQ(1u, list.size());
    target_layer = list[0];
    EXPECT_EQ(stub_handle_2->handle, list[0]->handle); 
}

//construction is a bit funny because hwc_layer_1 has unions
struct TestingHWCLayer : public hwc_layer_1
{
    TestingHWCLayer(
        int32_t compositionType,
        uint32_t hints,
        uint32_t flags,
        buffer_handle_t handle,
        uint32_t transform,
        int32_t blending,
        hwc_rect_t sourceCrop,
        hwc_rect_t displayFrame,
        hwc_region_t visibleRegionScreen,
        int acquireFenceFd,
        int releaseFenceFd)
    {
        hwc_layer_1::compositionType = compositionType;
        hwc_layer_1::hints = hints;
        hwc_layer_1::flags = flags;
        hwc_layer_1::handle = handle;
        hwc_layer_1::transform = transform;
        hwc_layer_1::blending = blending;
        hwc_layer_1::sourceCrop = sourceCrop;
        hwc_layer_1::displayFrame = displayFrame;
        hwc_layer_1::visibleRegionScreen = visibleRegionScreen;
        hwc_layer_1::acquireFenceFd = acquireFenceFd;
        hwc_layer_1::releaseFenceFd = releaseFenceFd;
    }
};

TEST_F(HWCLayerListTest, set_fb_target_programs_other_struct_members_correctly)
{
    using namespace testing;

    mga::HWCLayerList layerlist;
    layerlist.set_fb_target(mock_buffer);

    hwc_rect_t source_region = {0,0,width, height};
    hwc_rect_t target_region = source_region;
    TestingHWCLayer expected_layer{
                           HWC_FRAMEBUFFER_TARGET, //compositionType
                           0,            //hints
                           0,            //flags
                           stub_handle_1->handle, //handle
                           0,            //transform
                           HWC_BLENDING_NONE,      //blending
                           source_region, //source rect
                           target_region, //display position
                           {1,0},      //list of visible regions
                           -1,         //acquireFenceFd
                           -1};        //releaseFenceFd

    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list.size());
    auto target_layer = list[0];
    EXPECT_THAT(*target_layer, MatchesLayer( expected_layer ));
}
