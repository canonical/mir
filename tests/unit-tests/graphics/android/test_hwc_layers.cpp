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

#include "src/platforms/android/server/hwc_layerlist.h"
#include "mir/test/doubles/mock_buffer.h"
#include "hwc_struct_helpers.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir/test/doubles/mock_renderable.h"
#include "mir/test/fake_shared.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace
{

class HWCLayersTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        native_handle_1 = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>(buffer_size);
        ON_CALL(*mock_buffer, size())
            .WillByDefault(Return(buffer_size));
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_handle_1));

        list = std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>(
                ::operator new(sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t))));
        list_index = 0;
        hwc_layer = &list->hwLayers[list_index];
        type = mga::LayerType::gl_rendered;

        reset_expected_layer();
    }

    void reset_expected_layer()
    {
        memset(&expected_layer, 0, sizeof(expected_layer));
        expected_layer.compositionType = HWC_FRAMEBUFFER;
        expected_layer.hints = 0;
        expected_layer.flags = 0;
        expected_layer.handle = native_handle_1->handle();
        expected_layer.transform = 0;
        expected_layer.blending = HWC_BLENDING_NONE;
        expected_layer.sourceCrop = {
            0, 0,
            buffer_size.width.as_int(),
            buffer_size.height.as_int(),
        };
        expected_layer.displayFrame = {
            screen_position.top_left.x.as_int(),
            screen_position.top_left.y.as_int(),
            screen_position.bottom_right().x.as_int(),
            screen_position.bottom_right().y.as_int()};
        expected_layer.visibleRegionScreen = {1, &region};
        expected_layer.acquireFenceFd = -1;
        expected_layer.releaseFenceFd = -1;
        expected_layer.planeAlpha = std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max();
    }

    mga::LayerType type;
    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},buffer_size};
    bool alpha_enabled{false};
    std::shared_ptr<mtd::MockAndroidNativeBuffer> native_handle_1;
    std::shared_ptr<mtd::MockBuffer> mock_buffer{std::make_shared<testing::NiceMock<mtd::MockBuffer>>()};

    std::shared_ptr<hwc_display_contents_1_t> list;
    hwc_layer_1_t* hwc_layer;
    size_t list_index;
    hwc_layer_1 expected_layer;
    hwc_rect_t region{0,0,0,0};
    std::shared_ptr<mga::LayerAdapter> layer_adapter{std::make_shared<mga::IntegerSourceCrop>()};
};
}

TEST_F(HWCLayersTest, check_if_layer_needs_gl_render)
{
    mga::HWCLayer layer(layer_adapter, list, list_index);

    hwc_layer->compositionType = HWC_OVERLAY;
    hwc_layer->flags = 0;
    EXPECT_FALSE(layer.needs_gl_render());

    hwc_layer->compositionType = HWC_FRAMEBUFFER;
    hwc_layer->flags = HWC_SKIP_LAYER;
    EXPECT_TRUE(layer.needs_gl_render());

    hwc_layer->compositionType = HWC_FRAMEBUFFER;
    hwc_layer->flags = 0;
    EXPECT_TRUE(layer.needs_gl_render());
}

TEST_F(HWCLayersTest, move_layer_positions)
{
    mga::HWCLayer layer(layer_adapter, list, list_index, type, screen_position, false, mock_buffer);
    mga::HWCLayer second_layer(std::move(layer));

    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));
}

TEST_F(HWCLayersTest, change_layer_types)
{
    expected_layer.compositionType = HWC_FRAMEBUFFER_TARGET;

    type = mga::LayerType::framebuffer_target;
    mga::HWCLayer layer(
        layer_adapter, list, list_index,
        mga::LayerType::framebuffer_target,
        screen_position, alpha_enabled, mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));

    EXPECT_THROW({
        layer.setup_layer(
            mga::LayerType::overlay,
            screen_position,
            alpha_enabled,
            mock_buffer);
    }, std::logic_error);

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    layer.setup_layer(mga::LayerType::gl_rendered, screen_position, alpha_enabled, mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.flags = HWC_SKIP_LAYER;
    layer.setup_layer(mga::LayerType::skip, screen_position, alpha_enabled, mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.flags = 0;
    layer.setup_layer(mga::LayerType::gl_rendered, screen_position, alpha_enabled, mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));
}

TEST_F(HWCLayersTest, apply_buffer_updates_to_framebuffer_layer)
{
    EXPECT_CALL(*native_handle_1, copy_fence())
        .Times(0);

    hwc_rect_t region = {0,0,buffer_size.width.as_int(), buffer_size.height.as_int()};
    expected_layer.handle = native_handle_1->handle();
    expected_layer.visibleRegionScreen = {1, &region};
    expected_layer.sourceCrop = region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    mga::HWCLayer layer(layer_adapter, list, list_index, type, screen_position, alpha_enabled, mock_buffer);

    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));
}

TEST_F(HWCLayersTest, apply_buffer_updates_to_overlay_layers)
{
    int fake_fence = 552;
    hwc_rect_t region = {0,0,buffer_size.width.as_int(), buffer_size.height.as_int()};
    expected_layer.compositionType = HWC_OVERLAY;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.visibleRegionScreen = {1, &region};
    expected_layer.sourceCrop = region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    mga::HWCLayer layer(layer_adapter, list, list_index, type, screen_position, alpha_enabled, mock_buffer);
    hwc_layer->compositionType = HWC_OVERLAY;
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));

    //set_acquirefence should set the fence
    //mir must reset releaseFenceFd to -1
    hwc_layer->releaseFenceFd = fake_fence;
    EXPECT_CALL(*native_handle_1, copy_fence())
        .Times(1)
        .WillOnce(testing::Return(fake_fence));
    expected_layer.acquireFenceFd = fake_fence;
    layer.set_acquirefence();
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));
}

TEST_F(HWCLayersTest, apply_buffer_updates_to_fbtarget)
{
    int fake_fence = 552;
    hwc_rect_t region = {0,0,buffer_size.width.as_int(), buffer_size.height.as_int()};
    expected_layer.compositionType = HWC_FRAMEBUFFER_TARGET;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.visibleRegionScreen = {1, &region};
    expected_layer.sourceCrop = region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;

    mga::HWCLayer layer(
        layer_adapter, list, list_index,
        mga::LayerType::framebuffer_target,
        screen_position, alpha_enabled, mock_buffer);

    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));

    //mir must reset releaseFenceFd to -1 if hwc has changed it
    hwc_layer->releaseFenceFd = fake_fence;
    EXPECT_CALL(*native_handle_1, copy_fence())
        .Times(1)
        .WillOnce(testing::Return(fake_fence));
    expected_layer.acquireFenceFd = fake_fence;
    layer.set_acquirefence();
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));

    //hwc will set this to -1 to acknowledge that its adopted this layer's fence.
    //multiple sequential updates to the same layer must not set the acquireFenceFds on the calls
    //after the first.
    hwc_layer->acquireFenceFd = -1;
    expected_layer.acquireFenceFd = -1;
    layer.setup_layer(
        mga::LayerType::framebuffer_target,
        screen_position,
        false,
        mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));
}

TEST_F(HWCLayersTest, buffer_fence_updates)
{
    int fake_fence = 552;
    EXPECT_CALL(*native_handle_1, update_usage(fake_fence, mga::BufferAccess::read))
        .Times(1);
    mga::HWCLayer layer(
        layer_adapter, list, list_index,
        mga::LayerType::framebuffer_target,
        screen_position, alpha_enabled, mock_buffer);

    hwc_layer->releaseFenceFd = fake_fence;
    layer.release_buffer();
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
        screen_position.bottom_right().x.as_int(),
        screen_position.bottom_right().y.as_int()
    };

    hwc_region_t visible_region {1, &screen_pos};
    hwc_layer_1 expected_layer;

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_PREMULT;
    expected_layer.sourceCrop = crop;
    expected_layer.displayFrame = screen_pos;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;
    expected_layer.planeAlpha = std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max();

    mga::HWCLayer layer(layer_adapter, list, list_index);
    layer.setup_layer(
        mga::LayerType::gl_rendered,
        screen_position,
        true,
        mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));

    expected_layer.blending = HWC_BLENDING_NONE;
    layer.setup_layer(
        mga::LayerType::gl_rendered,
        screen_position,
        false,
        mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));
}

//HWC 1.3 and later started using floats for the sourceCrop field, and ints for the displayFrame
TEST_F(HWCLayersTest, has_float_sourcecrop)
{
    hwc_frect_t crop
    {
        0.0f, 0.0f,
        buffer_size.width.as_float(),
        buffer_size.height.as_float()
    };

    hwc_rect_t screen_pos
    {
        screen_position.top_left.x.as_int(),
        screen_position.top_left.y.as_int(),
        screen_position.bottom_right().x.as_int(),
        screen_position.bottom_right().y.as_int()
    };

    hwc_region_t visible_region {1, &screen_pos};
    hwc_layer_1 expected_layer;

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_PREMULT;
    expected_layer.sourceCropf = crop;
    expected_layer.displayFrame = screen_pos;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;
    expected_layer.planeAlpha = std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max();

    mga::HWCLayer layer(std::make_shared<mga::FloatSourceCrop>(), list, list_index);
    layer.setup_layer(
        mga::LayerType::gl_rendered,
        screen_position,
        true,
        mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLayer(expected_layer));
}

TEST_F(HWCLayersTest, list_can_be_offset_for_nonorigin_displays)
{
    using namespace testing;

    geom::Point offset{199, 299};

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
        screen_position.bottom_right().x.as_int(),
        screen_position.bottom_right().y.as_int()
    };

    hwc_region_t visible_region {1, &screen_pos};
    hwc_layer_1 expected_layer;

    expected_layer.compositionType = HWC_FRAMEBUFFER;
    expected_layer.hints = 0;
    expected_layer.flags = 0;
    expected_layer.handle = native_handle_1->handle();
    expected_layer.transform = 0;
    expected_layer.blending = HWC_BLENDING_PREMULT;
    expected_layer.sourceCrop = crop;
    expected_layer.displayFrame = screen_pos;
    expected_layer.visibleRegionScreen = visible_region;
    expected_layer.acquireFenceFd = -1;
    expected_layer.releaseFenceFd = -1;
    expected_layer.planeAlpha = std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max();

    mga::HWCLayer layer(layer_adapter, list, list_index);
    layer.setup_layer(
        mga::LayerType::gl_rendered,
        screen_position,
        true,
        mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));

    expected_layer.blending = HWC_BLENDING_NONE;
    layer.setup_layer(
        mga::LayerType::gl_rendered,
        screen_position,
        false,
        mock_buffer);
    EXPECT_THAT(*hwc_layer, MatchesLegacyLayer(expected_layer));
}
