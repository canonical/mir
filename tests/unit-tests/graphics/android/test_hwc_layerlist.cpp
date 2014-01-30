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
        ON_CALL(mock_renderable, buffer())
            .WillByDefault(Return(mt::fake_shared(mock_buffer)));
        ON_CALL(mock_renderable, alpha_enabled())
            .WillByDefault(Return(alpha_enabled));
        ON_CALL(mock_renderable, screen_position())
            .WillByDefault(Return(screen_position));
    }

    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},{245, 250}};
    bool alpha_enabled{true};
    int width{432};
    int height{876};
    mtd::StubAndroidNativeBuffer native_handle_1;
    testing::NiceMock<mtd::MockRenderable> mock_renderable;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;

    mga::ForceGLLayer skip_layer;
    mga::FramebufferLayer empty_target_layer;
};

TEST_F(HWCLayerListTest, list_defaults)
{
    mga::LayerList layerlist;

    layerlist.with_native_list([](hwc_display_contents_1_t& list)
    {
        EXPECT_EQ(-1, list.retireFenceFd);
        EXPECT_EQ(HWC_GEOMETRY_CHANGED, list.flags);
        EXPECT_NE(nullptr, list.dpy);
        EXPECT_NE(nullptr, list.sur);
    });
}

/* Tests without HWC_FRAMEBUFFER_TARGET */
/* an empty list should have a skipped layer. This will force a GL render on hwc set */
TEST_F(HWCLayerListTest, hwc10_list_defaults)
{
    mga::LayerList layerlist;

    layerlist.with_native_list([this](hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(1, list.numHwLayers);
        EXPECT_THAT(skip_layer, MatchesHWCLayer(list.hwLayers[0]));
    });
}

/* tests with a FRAMEBUFFER_TARGET present */
TEST_F(HWCLayerListTest, fbtarget_list_initialize)
{
    mga::FBTargetLayerList layerlist;
    layerlist.with_native_list([this](hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(2, list.numHwLayers);
        EXPECT_THAT(skip_layer, MatchesHWCLayer(list.hwLayers[0]));
        EXPECT_THAT(empty_target_layer, MatchesHWCLayer(list.hwLayers[1]));
    });
}

TEST_F(HWCLayerListTest, fb_target_set)
{
    mga::FBTargetLayerList layerlist;

    layerlist.set_fb_target(native_handle_1);

    layerlist.with_native_list([this](hwc_display_contents_1_t& list)
    {
        mga::ForceGLLayer forcegl_layer(native_handle_1);
        mga::FramebufferLayer fb_layer(native_handle_1);

        ASSERT_EQ(2, list.numHwLayers);
        EXPECT_THAT(forcegl_layer, MatchesHWCLayer(list.hwLayers[0]));
        EXPECT_THAT(fb_layer, MatchesHWCLayer(list.hwLayers[1]));
    });
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

    layerlist.with_native_list([this](hwc_display_contents_1_t& list)
    {
        mga::CompositionLayer comp_layer{mock_renderable}; 
        ASSERT_EQ(3, list.numHwLayers);
        EXPECT_THAT(comp_layer, MatchesHWCLayer(list.hwLayers[0]));
        EXPECT_THAT(comp_layer, MatchesHWCLayer(list.hwLayers[1]));
        EXPECT_THAT(empty_target_layer, MatchesHWCLayer(list.hwLayers[2]));
    });

    /* update FB target */
    layerlist.set_fb_target(native_handle_1);

    layerlist.with_native_list([this](hwc_display_contents_1_t& list)
    {
        mga::CompositionLayer comp_layer{mock_renderable};
        mga::FramebufferLayer fb_layer{native_handle_1};
        ASSERT_EQ(3, list.numHwLayers);
        EXPECT_THAT(comp_layer, MatchesHWCLayer(list.hwLayers[0]));
     //   EXPECT_THAT(comp_layer, MatchesHWCLayer(list.hwLayers[1]));
     //   EXPECT_THAT(fb_layer, MatchesHWCLayer(list.hwLayers[2]));
    });

#if 0
    /* reset default */
    layerlist.reset_composition_layers();

    layerlist.with_native_list([this](hwc_display_contents_1_t& list)
    {
//        hwc_layer_1_t layer_native, fb_native;
//        mga::ForceGLLayer skip_layer{&layer_native};
        mga::FramebufferLayer fb_layer{native_handle_1};
//        mga::FramebufferLayer fb_layer{&fb_native};

        ASSERT_EQ(2, list.numHwLayers);
        EXPECT_THAT(skip_layer, MatchesHWCLayer(list.hwLayers[0]));
        EXPECT_THAT(fb_layer, MatchesHWCLayer(list.hwLayers[1]));
    });
#endif
}

TEST_F(HWCLayerListTest, fb_fence)
{
    int release_fence = 381;
    mga::FBTargetLayerList layerlist;

    layerlist.with_native_list([this, &release_fence](hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(2, list.numHwLayers);
        list.hwLayers[1].releaseFenceFd = release_fence;
    });

    EXPECT_EQ(release_fence, layerlist.fb_target_fence());
}

TEST_F(HWCLayerListTest, retire_fence)
{
    int release_fence = 381;
    mga::FBTargetLayerList layerlist;

    layerlist.with_native_list([this, &release_fence](hwc_display_contents_1_t& list)
    {
        list.retireFenceFd = release_fence;
    });

    EXPECT_EQ(release_fence, layerlist.retirement_fence());
}
