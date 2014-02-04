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
        native_handle_1->anwb()->width = width;
        native_handle_1->anwb()->height = height;
        native_handle_2 = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();

        ON_CALL(mock_buffer, size())
            .WillByDefault(Return(buffer_size));
        ON_CALL(mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_handle_1));
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
    std::shared_ptr<mg::NativeBuffer> native_handle_1;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> native_handle_2;
    testing::NiceMock<mtd::MockRenderable> mock_renderable;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;
};

TEST_F(HWCLayersTest, fb_target_layer)
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

TEST_F(HWCLayersTest, fb_target_layer_no_buffer)
{
    mga::FramebufferLayer target_layer;

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
    expected_layer.displayFrame = region;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    EXPECT_THAT(target_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayersTest, normal_layer)
{
    using namespace testing;

    mga::CompositionLayer target_layer(mock_renderable); 

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
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_COVERAGE;
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

TEST_F(HWCLayersTest, normal_layer_no_alpha)
{
    using namespace testing;
    EXPECT_CALL(mock_renderable, alpha_enabled())
        .Times(1)
        .WillOnce(Return(false));

    mga::CompositionLayer target_layer(mock_renderable); 
    EXPECT_EQ(target_layer.blending, HWC_BLENDING_NONE);
}

TEST_F(HWCLayersTest, forced_gl_layer)
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

TEST_F(HWCLayersTest, forced_gl_layer_with_buffer)
{
    mga::ForceGLLayer target_layer(*native_handle_1);

    hwc_rect_t region = {0,0,width, height};
    hwc_region_t visible_region {1, &region};
    hwc_layer_1 expected_layer;
    memset(&expected_layer, 0, sizeof(expected_layer));

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
