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

        width = 432;
        height = 876;
        native_handle_1 = std::make_shared<mtd::StubAndroidNativeBuffer>();
        native_handle_1->anwb()->width = width;
        native_handle_1->anwb()->height = height;
        native_handle_2 = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();
    }

    int width;
    int height;
    std::shared_ptr<mg::NativeBuffer> native_handle_1;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> native_handle_2;
};

TEST_F(HWCLayerListTest, fb_target_layer)
{
    mga::FramebufferLayer target_layer(*native_handle_1);

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

    EXPECT_THAT(target_layer, MatchesLayer(expected_layer));
}

namespace mir
{
namespace test
{
namespace doubles
{
struct MockRenderable : public mg::Renderable
{
    MOCK_CONST_METHOD0(buffer, std::shared_ptr<mg::Buffer>());
    MOCK_CONST_METHOD0(alpha_enabled, bool());
    MOCK_CONST_METHOD0(screen_position, geom::Rectangle());
};
}
}
}

TEST_F(HWCLayerListTest, normal_layer)
{
    using namespace testing;
    geom::Size display_size{111, 222};
    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},{245, 250}};
    bool alpha_enabled = true;

    mtd::MockBuffer mock_buffer;
    EXPECT_CALL(mock_buffer, size())
        .Times(1)
        .WillOnce(Return(buffer_size));
    EXPECT_CALL(mock_buffer, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(native_handle_1));

    mtd::MockRenderable mock_renderable;
    EXPECT_CALL(mock_renderable, buffer())
        .Times(1)
        .WillOnce(Return(mt::fake_shared(mock_buffer)));
    EXPECT_CALL(mock_renderable, alpha_enabled())
        .Times(1)
        .WillOnce(Return(alpha_enabled));
    EXPECT_CALL(mock_renderable, screen_position())
        .Times(1)
        .WillOnce(Return(screen_position));

    mga::CompositionLayer target_layer(mock_renderable, display_size); 

    hwc_rect_t screen_region
    {
        0, 0,
        static_cast<int>(display_size.width.as_uint32_t()),
        static_cast<int>(display_size.height.as_uint32_t())
    };

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

    hwc_region_t visible_region {1, &screen_region};
    hwc_layer_1 expected_layer;

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_NONE;
    expected_layer.sourceCrop = crop;
    expected_layer.displayFrame = screen_pos;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    EXPECT_THAT(target_layer, MatchesLayer(expected_layer));
    EXPECT_TRUE(target_layer.needs_gl_render());

    target_layer.compositionType = HWC_OVERLAY;
    EXPECT_FALSE(target_layer.needs_gl_render());
}

TEST_F(HWCLayerListTest, normal_layer_no_alpha)
{
    using namespace testing;
    geom::Size display_size{111, 222};
    bool alpha_enabled = false;
    mtd::MockRenderable mock_renderable;
    EXPECT_CALL(mock_renderable, alpha_enabled())
        .Times(1)
        .WillOnce(Return(alpha_enabled));

    mga::CompositionLayer target_layer(mock_renderable, display_size); 
    EXPECT_EQ(target_layer.blending, HWC_BLENDING_COVERAGE);
}

TEST_F(HWCLayerListTest, forced_gl_layer)
{
    mga::ForceGLLayer target_layer;

    hwc_rect_t region = {0,0,0,0};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = HWC_SKIP_LAYER;
    expected_layer.handle = nullptr;
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_NONE;
    expected_layer.sourceCrop = region;
    expected_layer.displayFrame = region;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    EXPECT_THAT(target_layer, MatchesLayer(expected_layer));
    EXPECT_TRUE(target_layer.needs_gl_render());

    target_layer.compositionType = HWC_OVERLAY;
    EXPECT_TRUE(target_layer.needs_gl_render());
}

#if 0
TEST_F(HWCLayerListTest, hwc_list_creation)
{
    using namespace testing;

    mga::CompositionLayer surface_layer(*native_handle_1, 0);
    mga::FramebufferLayer target_layer(*native_handle_1);
    mga::LayerList layerlist({
        surface_layer,
        target_layer});

    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    /* note, mali hwc1.1 actually falsely returns if these are not set to something. set to garbage */
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);

    ASSERT_EQ(2u, list->numHwLayers);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, hwc_list_update)
{
    using namespace testing;

    int handle_fence = 442;
    EXPECT_CALL(*native_handle_2, copy_fence())
        .Times(1)
        .WillOnce(Return(handle_fence));

    mga::LayerList layerlist({
        mga::CompositionLayer(*native_handle_1, 0),
        mga::FramebufferLayer(*native_handle_1)});
    layerlist.set_fb_target(native_handle_2);

    auto list = layerlist.native_list();
    ASSERT_EQ(2u, list->numHwLayers);
    EXPECT_EQ(native_handle_1->handle(), list->hwLayers[0].handle);
    EXPECT_EQ(-1, list->hwLayers[0].acquireFenceFd);
    EXPECT_EQ(list->hwLayers[1].handle, native_handle_2->handle());
    EXPECT_EQ(handle_fence, list->hwLayers[1].acquireFenceFd);
}

TEST_F(HWCLayerListTest, get_fb_fence)
{
    int release_fence = 381;
    mga::LayerList layerlist({
        mga::CompositionLayer(*native_handle_1, 0),
        mga::FramebufferLayer(*native_handle_1)});

    auto list = layerlist.native_list();
    list->hwLayers[1].releaseFenceFd = release_fence;

    EXPECT_EQ(release_fence, layerlist.framebuffer_fence());
}
#endif
