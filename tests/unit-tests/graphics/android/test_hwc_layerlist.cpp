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
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
//MATCHER



class HWCLayerListTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        mock_buffer = std::make_shared<mtd::MockBuffer>();
    }

    std::shared_ptr<mtd::MockBuffer> mock_buffer;
};

void PrintTo(const hwc_layer_1& layer , ::std::ostream* os)
{
  *os << "compositionType: " << layer.compositionType << std::endl
    << "\thints: " << layer.hints << std::endl
    << "\tflags: " << layer.flags << std::endl
    << "\thandle: " << layer.handle << std::endl
    << "\ttransform: " << layer.transform << std::endl
    << "\tblending: " << layer.blending << std::endl
    << "\tsourceCrop: (" << "left: "  << layer.sourceCrop.left
                           << " top: "   << layer.sourceCrop.top
                           << " right "  << layer.sourceCrop.right
                           << " bottom: "<< layer.sourceCrop.bottom << ")" << std::endl
    << "\tdisplayFrame: ("<<"left: "  << layer.displayFrame.left
                           << " top: "   << layer.displayFrame.top
                           << " right "  << layer.displayFrame.right
                           << " bottom: "<< layer.displayFrame.bottom << ")" << std::endl
    << "\tvisibleRegionScreen.numRects: " << layer.visibleRegionScreen.numRects << std::endl
    << "\tacquireFenceFd: " << layer.acquireFenceFd << std::endl
    << "\treleaseFenceFd: " << layer.releaseFenceFd << std::endl;
}

MATCHER_P2(MatchesMember, value, str,
          std::string("layer's " + std::string(str) + " should be: " + testing::PrintToString(value)))
{
    return arg == value;
}
MATCHER_P(MatchesLayer, value, std::string(testing::PrintToString(value)) )
{
    auto layer = arg; //makes for nicer printing
    EXPECT_THAT(layer.compositionType, MatchesMember(value.compositionType, "compositionType"));
    EXPECT_THAT(layer.hints, MatchesMember(value.hints, "hints"));
    EXPECT_THAT(layer.flags, MatchesMember(value.flags, "flags"));
    EXPECT_THAT(layer.handle, MatchesMember(value.handle, "backgroundColor.handle"));
    EXPECT_THAT(layer.transform, MatchesMember(value.transform, "backgroundColor.transform"));
    EXPECT_THAT(layer.blending, MatchesMember(value.blending, "backgroundColor.blending"));
    EXPECT_THAT(layer.sourceCrop.left, MatchesMember(value.sourceCrop.left, "sourceCrop.left"));
    EXPECT_THAT(layer.sourceCrop.top, MatchesMember(value.sourceCrop.top, "sourceCrop.top"));
    EXPECT_THAT(layer.sourceCrop.right, MatchesMember(value.sourceCrop.right, "sourceCrop.right"));
    EXPECT_THAT(layer.sourceCrop.bottom, MatchesMember(value.sourceCrop.bottom, "sourceCrop.bottom"));
    EXPECT_THAT(layer.displayFrame.left, MatchesMember(value.displayFrame.left, "displayFrame.left"));
    EXPECT_THAT(layer.displayFrame.top, MatchesMember(value.displayFrame.top, "displayFrame.top"));
    EXPECT_THAT(layer.displayFrame.right, MatchesMember(value.displayFrame.right, "displayFrame.right"));
    EXPECT_THAT(layer.displayFrame.bottom, MatchesMember(value.displayFrame.bottom, "displayFrame.bottom"));
    EXPECT_THAT(layer.visibleRegionScreen.numRects, MatchesMember(value.visibleRegionScreen.numRects, "visibleRegionScreen.numRects"));
    EXPECT_THAT(layer.acquireFenceFd, MatchesMember(value.acquireFenceFd, "acquireFenceFd"));
    EXPECT_THAT(layer.releaseFenceFd, MatchesMember(value.releaseFenceFd, "releaseFenceFd"));

    return !(::testing::Test::HasFailure());
}

TEST_F(HWCLayerListTest, matcher)
{
    hwc_layer_1_t good_layer;
    memset(&good_layer, 0, sizeof(hwc_layer_1_t));
    good_layer.compositionType = 2;

    hwc_layer_1_t layer;
    memset(&layer, 0, sizeof(hwc_layer_1_t));
    layer.compositionType = 3;
    EXPECT_THAT(layer, MatchesLayer( good_layer ));
}

TEST_F(HWCLayerListTest, empty_list)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    auto list = layerlist.native_list(); 
    EXPECT_EQ(0u, list.size());
}

struct HWCLayer : public hwc_layer_1
{
    HWCLayer(
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
        compositionType = compositionType;
        hints = hints;
        flags = flags;
        handle = handle;
        transform = transform;
        blending = blending;
        sourceCrop = sourceCrop;
        displayFrame = displayFrame;
        visibleRegionScreen = visibleRegionScreen;
        acquireFenceFd = acquireFenceFd;
        releaseFenceFd = releaseFenceFd;
    }
};

TEST_F(HWCLayerListTest, set_fb_target_figures_out_buffer_size)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    EXPECT_CALL(*mock_buffer, size())
        .Times(1)
        .WillOnce(Return(default_size));

    layerlist.set_fb_target(mock_buffer);

    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list.size());
    ASSERT_EQ(1u, *list[0].visibleRegionScreen.numRects); 
    ASSERT_NE(nullptr, *list[0].visibleRegionScreen.rects); 

    EXPECT_THAT(*list[0].sourceCrop, MatchesRect( expected_sc ));
    EXPECT_THAT(*list[0].displayFrame, MatchesRect( expected_df ));
    EXPECT_THAT(*(*list[0].visibleScreenRegion).rects, MatchesRect( expected_visible ));
}

TEST_F(HWCLayerListTest, set_fb_target_gets_fb_handle)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    EXPECT_CALL(mock_buffer, get_native_handle)
        .Times(1)
        .WillOnce(Return(dummy_handle));

    layerlist.set_fb_target(mock_buffer);
    auto list = layerlist.native_list(); 
    EXPECT_EQ(dummy_handle, *list[0].handle); 
}

TEST_F(HWCDevice, set_fb_target_2x)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    buffer_handle_t dummy_handle_1 = (buffer_handle_t) 0x4838333;
    buffer_handle_t dummy_handle_2 = (buffer_handle_t) 0x1853;

    EXPECT_CALL(mock_buffer, get_native_handle)
        .Times(2)
        .WillOnce(Return(dummy_handle_1))
        .WillOnce(Return(dummy_handle_2));

    layerlist.set_fb_target(mock_buffer);
    auto list = layerlist.native_list(); 
    EXPECT_EQ(dummy_handle_1, *list[0].handle); 

    layerlist.set_fb_target(mock_buffer);
    auto list_second = layerlist.native_list();

    EXPECT_EQ(list, list_second); 
    EXPECT_EQ(dummy_handle_2, *list_second[0].handle); 
}


TEST_F(HWCLayerListTest, set_fb_target_programs_other_struct_members_correctly)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    layerlist.set_fb_target(mock_buffer);

    auto list = layerlist.native_list(); 
    ASSERT_EQ(1u, list.size());

    buffer_handle_t dummy_handle = (buffer_handle_t) 0x4838333;

    hwc_rect_t source_region = {0,0,422,223};
    hwc_rect_t target_region = {0,0,422,223};
    HWCLayer expected_layer{
                           HWC_FRAMEBUFFER_TARGET, //compositionType
                           0,            //hints
                           0,            //flags
                           dummy_handle, //handle
                           0,            //transform
                           HWC_BLENDING_NONE,      //blending
                           source_region, //source rect
                           target_region, //display position
                           {0,0},      //list of visible regions
                           -1,         //acquireFenceFd
                           -1};        //releaseFenceFd

    EXPECT_THAT(*list[0], MatchesLayer( expected_layer ));
}
