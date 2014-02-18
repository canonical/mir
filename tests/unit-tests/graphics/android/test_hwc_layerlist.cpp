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
#include "mir_test_doubles/mock_render_function.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace
{

class StubRenderable : public mg::Renderable
{
public:
    StubRenderable(std::shared_ptr<mg::Buffer> const& buffer, geom::Rectangle screen_pos)
        : buf(buffer),
          screen_pos(screen_pos)
    {
    }

    std::shared_ptr<mg::Buffer> buffer() const
    {
        return buf;
    }

    bool alpha_enabled() const
    {
        return false;
    }

    geom::Rectangle screen_position() const override
    {
        return screen_pos;
    }

private:
    std::shared_ptr<mg::Buffer> buf;
    geom::Rectangle screen_pos;
};

}

class HWCLayerListTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        ON_CALL(mock_buffer, size())
            .WillByDefault(Return(buffer_size));
        ON_CALL(mock_buffer, native_buffer_handle())
            .WillByDefault(Return(mt::fake_shared(native_handle_1)));

        stub_renderable1 = std::make_shared<StubRenderable>(
                                mt::fake_shared(mock_buffer), screen_position);
        stub_renderable2 = std::make_shared<StubRenderable>(
                                mt::fake_shared(mock_buffer), screen_position);

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
        comp_layer.blending = HWC_BLENDING_NONE;
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

        empty_prepare_fn = [] (hwc_display_contents_1_t&) {};
        empty_render_fn = [] (mg::Renderable const&) {};
    }

    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},{245, 250}};
    mtd::StubAndroidNativeBuffer native_handle_1{buffer_size};
    testing::NiceMock<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<StubRenderable> stub_renderable1;
    std::shared_ptr<StubRenderable> stub_renderable2;

    std::function<void(hwc_display_contents_1_t&)> empty_prepare_fn;
    std::function<void(mg::Renderable const&)> empty_render_fn;

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
        stub_renderable1,
        stub_renderable1
    });

    layerlist.prepare_composition_layers(empty_prepare_fn, updated_list, empty_render_fn);
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
    layerlist.prepare_default_layers(empty_prepare_fn);
    EXPECT_TRUE(layerlist.needs_swapbuffers());

    list = layerlist.native_list().lock();
    target_layer.handle = nullptr;
    ASSERT_EQ(2, list->numHwLayers);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fence_updates)
{
    int release_fence1 = 381;
    int release_fence2 = 382;
    int release_fence3 = 383;
    auto native_handle_1 = std::make_shared<mtd::StubAndroidNativeBuffer>();
    auto native_handle_2 = std::make_shared<mtd::StubAndroidNativeBuffer>();
    auto native_handle_3 = std::make_shared<mtd::StubAndroidNativeBuffer>();
    EXPECT_CALL(*native_handle_1, update_fence(release_fence1))
        .Times(1);
    EXPECT_CALL(*native_handle_2, update_fence(release_fence2))
        .Times(1);
    EXPECT_CALL(*native_handle_3, update_fence(release_fence3))
        .Times(1);

    EXPECT_CALL(mock_buffer, native_buffer_handle())
        .WillOnce(Return(native_handle_1))
        .WillOnce(Return(native_handle_2))
        .WillRepeatedly(Return(native_handle_3));

    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    mga::FBTargetLayerList layerlist;
    layerlist.prepare_composition_layers(empty_prepare_fn, updated_list, empty_render_fn);
    layerlist.set_fb_target(mock_buffer);

    auto list = layerlist.native_list().lock();
    ASSERT_EQ(3, list->numHwLayers);
    list->hwLayers[0].releaseFenceFd = release_fence1;
    list->hwLayers[1].releaseFenceFd = release_fence2;
    list->hwLayers[2].releaseFenceFd = release_fence3;

    layerlist.update_fences();
}

TEST_F(HWCLayerListTest, list_returns_retire_fence)
{
    int release_fence = 381;
    mga::FBTargetLayerList layerlist;

    auto list = layerlist.native_list().lock();
    list->retireFenceFd = release_fence;
    EXPECT_EQ(release_fence, layerlist.retirement_fence());
}

TEST_F(HWCLayerListTest, list_prepares_and_renders)
{
    using namespace testing;
    mtd::MockRenderFunction mock_call_counter;
    auto render_fn = [&] (mg::Renderable const& renderable)
    {
        mock_call_counter.called(renderable);
    };

    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    auto prepare_fn_all_gl = [] (hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(3, list.numHwLayers);
        list.hwLayers[0].compositionType = HWC_FRAMEBUFFER;
        list.hwLayers[1].compositionType = HWC_FRAMEBUFFER;
        list.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
    };

    auto prepare_fn_mixed = [] (hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(3, list.numHwLayers);
        list.hwLayers[0].compositionType = HWC_OVERLAY;
        list.hwLayers[1].compositionType = HWC_FRAMEBUFFER;
        list.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
    };

    auto prepare_fn_all_overlay = [] (hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(3, list.numHwLayers);
        list.hwLayers[0].compositionType = HWC_OVERLAY;
        list.hwLayers[1].compositionType = HWC_OVERLAY;
        list.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
    };

    mga::FBTargetLayerList layerlist;

    Sequence seq;
    EXPECT_CALL(mock_call_counter, called(Ref(*stub_renderable1)))
        .InSequence(seq);
    EXPECT_CALL(mock_call_counter, called(Ref(*stub_renderable2)))
        .InSequence(seq);
    EXPECT_CALL(mock_call_counter, called(Ref(*stub_renderable2)))
        .InSequence(seq);

    layerlist.prepare_composition_layers(prepare_fn_all_gl, updated_list, render_fn);
    EXPECT_TRUE(layerlist.needs_swapbuffers());

    layerlist.prepare_composition_layers(prepare_fn_mixed, updated_list, render_fn);
    EXPECT_TRUE(layerlist.needs_swapbuffers());

    layerlist.prepare_composition_layers(prepare_fn_all_overlay, updated_list, render_fn);
    EXPECT_FALSE(layerlist.needs_swapbuffers());
}
