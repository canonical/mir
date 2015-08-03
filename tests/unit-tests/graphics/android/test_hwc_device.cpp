/**
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

#include "mir/graphics/android/sync_fence.h"
#include "src/platforms/android/server/framebuffer_bundle.h"
#include "src/platforms/android/server/hwc_device.h"
#include "src/platforms/android/server/hwc_layerlist.h"
#include "src/platforms/android/server/gl_context.h"
#include "src/platforms/android/server/hwc_configuration.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/doubles/mock_framebuffer_bundle.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/mock_hwc_device_wrapper.h"
#include "mir/test/fake_shared.h"
#include "hwc_struct_helpers.h"
#include "mir/test/doubles/mock_swapping_gl_context.h"
#include "mir/test/doubles/stub_swapping_gl_context.h"
#include "mir/test/doubles/stub_renderable_list_compositor.h"
#include "mir/test/doubles/mock_renderable_list_compositor.h"
#include "mir/test/doubles/mock_renderable.h"
#include "mir/test/doubles/stub_renderable.h"
#include <unistd.h>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;
namespace mt=mir::test;

namespace
{
struct MockFileOps : public mga::SyncFileOps
{
    MOCK_METHOD3(ioctl, int(int,int,void*));
    MOCK_METHOD1(dup, int(int));
    MOCK_METHOD1(close, int(int));
};

struct HwcDevice : public ::testing::Test
{
    HwcDevice() :
        mock_native_buffer1(std::make_shared<testing::NiceMock<mtd::MockAndroidNativeBuffer>>(size1)),
        mock_native_buffer2(std::make_shared<testing::NiceMock<mtd::MockAndroidNativeBuffer>>(size2)),
        mock_native_buffer3(std::make_shared<testing::NiceMock<mtd::MockAndroidNativeBuffer>>(size3)),
        stub_buffer1(std::make_shared<mtd::StubBuffer>(mock_native_buffer1, size1)),
        stub_buffer2(std::make_shared<mtd::StubBuffer>(mock_native_buffer2, size2)),
        stub_fb_buffer(std::make_shared<mtd::StubBuffer>(mock_native_buffer3, size3)),
        stub_renderable1(std::make_shared<mtd::StubRenderable>(stub_buffer1, position1)),
        stub_renderable2(std::make_shared<mtd::StubRenderable>(stub_buffer2, position2)),
        mock_device(std::make_shared<testing::NiceMock<mtd::MockHWCDeviceWrapper>>()),
        stub_context{stub_fb_buffer},
        renderlist({stub_renderable1, stub_renderable2}),
        layer_adapter{std::make_shared<mga::IntegerSourceCrop>()}
    {
        mt::fill_hwc_layer(layer, &comp_rect, position1, *stub_buffer1, HWC_FRAMEBUFFER, 0);
        mt::fill_hwc_layer(layer2, &comp2_rect, position2, *stub_buffer2, HWC_FRAMEBUFFER, 0);
        mt::fill_hwc_layer(target_layer, &target_rect, fb_position, *stub_fb_buffer, HWC_FRAMEBUFFER_TARGET, 0);
        mt::fill_hwc_layer(skip_layer, &skip_rect, fb_position, *stub_fb_buffer, HWC_FRAMEBUFFER, HWC_SKIP_LAYER);
        set_all_layers_to_overlay = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
        {
            for(auto i = 0u; i < contents[0]->numHwLayers - 1; i++) //-1 because the last layer is the target
                contents[0]->hwLayers[i].compositionType = HWC_OVERLAY;
        };
        reject_all_layers = [](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&){};
    }

    hwc_rect_t skip_rect;
    hwc_rect_t target_rect;
    hwc_rect_t comp_rect;
    hwc_rect_t comp2_rect;
    hwc_layer_1_t skip_layer;
    hwc_layer_1_t target_layer;
    hwc_layer_1_t layer;
    hwc_layer_1_t layer2;

    geom::Size const size1{111, 222};
    geom::Size const size2{333, 444};
    geom::Size const size3{555, 666};
    geom::Rectangle const position1{{44,1},size1};
    geom::Rectangle const position2{{92,293},size2};
    geom::Rectangle const fb_position{{0,0},size3};
    mtd::StubRenderableListCompositor stub_compositor;
    std::function<void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)> set_all_layers_to_overlay;
    std::function<void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)> reject_all_layers;

    std::shared_ptr<mtd::MockAndroidNativeBuffer> const mock_native_buffer1;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> const mock_native_buffer2;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> const mock_native_buffer3;
    std::shared_ptr<mtd::StubBuffer> const stub_buffer1;
    std::shared_ptr<mtd::StubBuffer> const stub_buffer2;
    std::shared_ptr<mtd::StubBuffer> const stub_fb_buffer;
    std::shared_ptr<mtd::StubRenderable> const stub_renderable1;
    std::shared_ptr<mtd::StubRenderable> const stub_renderable2;
    std::shared_ptr<mtd::MockHWCDeviceWrapper> const mock_device;
    mtd::StubSwappingGLContext stub_context;
    mg::RenderableList renderlist;
    std::shared_ptr<mga::LayerAdapter> const layer_adapter;
    mga::DisplayName primary{mga::DisplayName::primary};
    geom::Displacement offset;
};
}

TEST_F(HwcDevice, prepares_a_skip_and_target_layer_by_default)
{
    using namespace testing;
    std::list<hwc_layer_1_t*> expected_list
    {
        &skip_layer,
        &target_layer
    };

    EXPECT_CALL(*mock_device, prepare(MatchesPrimaryList(expected_list)))
        .Times(1);

    mga::LayerList list(layer_adapter, {}, geom::Displacement{0,0});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    mga::HwcDevice device(mock_device);
    device.commit({content});
}

TEST_F(HwcDevice, calls_backup_compositor_when_overlay_rejected)
{
    using namespace testing;
    mtd::MockRenderableListCompositor mock_compositor;

    mg::RenderableList expected_renderable_list({
        stub_renderable2
    });

    std::list<hwc_layer_1_t*> expected_prepare_list
    {
        &layer,
        &layer2,
        &target_layer
    };

    Sequence seq;
    EXPECT_CALL(*mock_device, prepare(MatchesPrimaryList(expected_prepare_list)))
        .InSequence(seq)
        .WillOnce(Invoke([&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
        {
            ASSERT_EQ(contents[0]->numHwLayers, 3);
            contents[0]->hwLayers[0].compositionType = HWC_OVERLAY;
            contents[0]->hwLayers[1].compositionType = HWC_FRAMEBUFFER;
            contents[0]->hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));

    EXPECT_CALL(mock_compositor, render(expected_renderable_list, offset, Ref(stub_context)))
        .InSequence(seq);

    mga::LayerList list(layer_adapter, renderlist, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, mock_compositor};
    mga::HwcDevice device(mock_device);
    device.commit({content});
}

TEST_F(HwcDevice, swaps_buffers_directly_when_no_renderables)
{
    using namespace testing;
    mtd::MockRenderableListCompositor mock_compositor;
    mtd::MockSwappingGLContext mock_context;
    ON_CALL(mock_context, last_rendered_buffer())
        .WillByDefault(Return(stub_fb_buffer));

    EXPECT_CALL(mock_compositor, render(_,_,_))
        .Times(0);
    EXPECT_CALL(mock_context, swap_buffers());

    mga::LayerList list(layer_adapter, {}, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, mock_context, mock_compositor};
    mga::HwcDevice device(mock_device);
    device.commit({content});
}

TEST_F(HwcDevice, resets_layers_when_prepare_gl_called)
{
    using namespace testing;
    std::list<hwc_layer_1_t*> expected_list1
    {
        &layer,
        &layer2,
        &target_layer
    };

    std::list<hwc_layer_1_t*> expected_list2
    {
        &skip_layer,
        &target_layer
    };

    Sequence seq;
    EXPECT_CALL(*mock_device, prepare(MatchesPrimaryList(expected_list1)))
        .InSequence(seq);
    EXPECT_CALL(*mock_device, prepare(MatchesPrimaryList(expected_list2)))
        .InSequence(seq);

    mga::HwcDevice device(mock_device);
    mga::LayerList list(layer_adapter, renderlist, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    list.update_list({}, geom::Displacement{});
    device.commit({content});
}

TEST_F(HwcDevice, sets_and_updates_fences)
{
    using namespace testing;
    int fb_release_fence = 94;
    int hwc_retire_fence = ::open("/dev/null", 0);
    int* list_retire_fence = nullptr;
    auto set_fences_fn = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
    {
        ASSERT_EQ(contents[0]->numHwLayers, 2);
        contents[0]->hwLayers[1].releaseFenceFd = fb_release_fence;
        contents[0]->retireFenceFd = hwc_retire_fence;
        list_retire_fence = &contents[0]->retireFenceFd;
    };

    std::list<hwc_layer_1_t*> expected_list
    {
        &skip_layer,
        &target_layer
    };

    Sequence seq;
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences_fn));
    EXPECT_CALL(*mock_native_buffer3, update_usage(fb_release_fence, mga::BufferAccess::read))
        .InSequence(seq);

    mga::HwcDevice device(mock_device);
    mga::LayerList list(layer_adapter, {}, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});

    //check that the retire fence is closed
    bool retire_fence_was_closed{fcntl(hwc_retire_fence, F_GETFD) == -1};
    EXPECT_TRUE(retire_fence_was_closed);
    if (!retire_fence_was_closed)
        close(hwc_retire_fence);
}

TEST_F(HwcDevice, commits_correct_list_with_rejected_renderables)
{
    using namespace testing;
    int fb_acquire_fence = 80;
    int fb_release_fence = 383;

    auto set_fences_fn = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
    {
        ASSERT_EQ(contents[0]->numHwLayers, 2);
        contents[0]->hwLayers[1].releaseFenceFd = fb_release_fence;
        contents[0]->retireFenceFd = -1;
    };

    layer.acquireFenceFd = -1;
    target_layer.acquireFenceFd = fb_acquire_fence;

    std::list<hwc_layer_1_t*> expected_list
    {
        &layer,
        &target_layer
    };


    EXPECT_CALL(*mock_native_buffer1, copy_fence())
        .Times(0);
    EXPECT_CALL(*mock_native_buffer1, update_usage(_,_))
        .Times(0);
    Sequence seq; 
    EXPECT_CALL(*mock_device, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke(reject_all_layers));
    EXPECT_CALL(*mock_native_buffer3, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(fb_acquire_fence));
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences_fn));
    EXPECT_CALL(*mock_native_buffer3, update_usage(fb_release_fence, mga::BufferAccess::read))
        .InSequence(seq);

    mga::HwcDevice device(mock_device);
    mga::LayerList list(layer_adapter, {stub_renderable1}, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
}

TEST_F(HwcDevice, commits_correct_list_when_all_accepted_as_overlays)
{
    using namespace testing;
    int overlay_acquire_fence1 = 80;
    int overlay_acquire_fence2 = 81;
    int release_fence1 = 381;
    int release_fence2 = 382;

    auto set_fences_fn = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
    {
        ASSERT_EQ(contents[0]->numHwLayers, 3);
        contents[0]->hwLayers[0].releaseFenceFd = release_fence1;
        contents[0]->hwLayers[1].releaseFenceFd = release_fence2;
        contents[0]->retireFenceFd = -1;
    };

    layer.compositionType = HWC_OVERLAY;
    layer.acquireFenceFd = overlay_acquire_fence1;

    layer2.compositionType = HWC_OVERLAY;
    layer2.acquireFenceFd = overlay_acquire_fence2;

    //all layers are overlays, so we don't copy the fence. lp: #1331769
    target_layer.acquireFenceFd = -1;

    std::list<hwc_layer_1_t*> expected_list
    {
        &layer,
        &layer2,
        &target_layer
    };

    EXPECT_CALL(*mock_native_buffer3, copy_fence())
        .Times(0);
    int invalid{-1};
    EXPECT_CALL(*mock_native_buffer3, update_usage(invalid,_))
        .Times(1);

    Sequence seq; 
    EXPECT_CALL(*mock_device, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke(set_all_layers_to_overlay));

    //copy all fb fences for OVERLAY or FRAMEBUFFER_TARGET in preparation of set
    EXPECT_CALL(*mock_native_buffer1, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(overlay_acquire_fence1));
    EXPECT_CALL(*mock_native_buffer2, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(overlay_acquire_fence2));
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences_fn));
    EXPECT_CALL(*mock_native_buffer1, update_usage(release_fence1, mga::BufferAccess::read))
        .InSequence(seq);
    EXPECT_CALL(*mock_native_buffer2, update_usage(release_fence2, mga::BufferAccess::read))
        .InSequence(seq);

    mga::HwcDevice device(mock_device);
    mga::LayerList list(layer_adapter, renderlist, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
}

TEST_F(HwcDevice, submits_every_time_if_at_least_one_layer_is_gl_rendered)
{
    using namespace testing;
    mga::HwcDevice device(mock_device);

    ON_CALL(*mock_device, prepare(_))
        .WillByDefault(Invoke([&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
        {
            ASSERT_EQ(contents[0]->numHwLayers, 3);
            contents[0]->hwLayers[0].compositionType = HWC_OVERLAY;
            contents[0]->hwLayers[1].compositionType = HWC_FRAMEBUFFER;
            contents[0]->hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));

    EXPECT_CALL(*mock_device, set(_))
        .Times(2);

    mga::LayerList list(layer_adapter, renderlist, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    list.update_list(renderlist, geom::Displacement{});
    device.commit({content});
}

TEST_F(HwcDevice, resets_composition_type_with_prepare) //lp:1314399
{
    using namespace testing;
    mg::RenderableList renderlist({stub_renderable1});
    mg::RenderableList renderlist2({stub_renderable2});
    mga::HwcDevice device(mock_device);

    std::list<hwc_layer_1_t*> expected_list1 { &layer, &target_layer };
    std::list<hwc_layer_1_t*> expected_list2 { &layer2, &target_layer };

    Sequence seq; 
    EXPECT_CALL(*mock_device, prepare(MatchesPrimaryList(expected_list1)))
        .InSequence(seq)
        .WillOnce(Invoke(set_all_layers_to_overlay));
    EXPECT_CALL(*mock_device, prepare(MatchesPrimaryList(expected_list2)))
        .InSequence(seq);

    mga::LayerList list(layer_adapter, renderlist, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    list.update_list(renderlist2, geom::Displacement{});
    device.commit({content});
}

//note: HWC models overlay layer buffers as owned by the display hardware until a subsequent set.
TEST_F(HwcDevice, owns_overlay_buffers_until_next_set)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare(_))
        .WillOnce(Invoke(set_all_layers_to_overlay))
        .WillOnce(Return());

    mga::HwcDevice device(mock_device);

    auto use_count_before = stub_buffer1.use_count();
    mga::LayerList list(layer_adapter, {stub_renderable1}, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    EXPECT_THAT(stub_buffer1.use_count(), Gt(use_count_before));
    list.update_list({stub_renderable1}, geom::Displacement{});
    device.commit({content});
    EXPECT_THAT(stub_buffer1.use_count(), Eq(use_count_before));
}

TEST_F(HwcDevice, overlays_are_throttled_per_predictive_bypass)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare(_))
        .WillRepeatedly(Invoke(set_all_layers_to_overlay));

    mga::HwcDevice device(mock_device);

    mga::LayerList list(layer_adapter, {stub_renderable1}, {0,0});
    mga::DisplayContents content{primary, list, offset, stub_context,
                                 stub_compositor};

    for (int frame = 0; frame < 5; ++frame)
    {
        device.commit({content});
        ASSERT_THAT(device.recommended_sleep().count(), Ge(8));
    }
}

TEST_F(HwcDevice, compositing_disables_predictive_bypass)
{
    using namespace testing;

    NiceMock<mtd::MockSwappingGLContext> mock_context;
    ON_CALL(mock_context, last_rendered_buffer())
        .WillByDefault(Return(stub_fb_buffer));
    EXPECT_CALL(mock_context, swap_buffers())
        .Times(AtLeast(5));

    mga::LayerList list(layer_adapter, {}, geom::Displacement{});
    mtd::MockRenderableListCompositor mock_compositor;
    mga::DisplayContents content{primary, list, offset, mock_context,
                                 mock_compositor};

    mga::HwcDevice device(mock_device);
    device.commit({content});
    for (int frame = 0; frame < 5; ++frame)
    {
        device.commit({content});
        ASSERT_EQ(0, device.recommended_sleep().count());
    }
}

TEST_F(HwcDevice, does_not_set_acquirefences_when_it_has_set_them_previously_without_update)
{
    using namespace testing;
    int acquire_fence1 = 39303;
    int acquire_fence2 = 39302;
    int acquire_fence3 = 39301;

    int release_fence1 = 381;
    int release_fence2 = 382;
    auto native_buffer = std::make_shared<testing::NiceMock<mtd::MockAndroidNativeBuffer>>(size1);
    auto updated_buffer = std::make_shared<mtd::StubBuffer>(native_buffer, size1);
    auto set_fences_fn = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
    {
        ASSERT_EQ(contents[0]->numHwLayers, 3);
        contents[0]->hwLayers[0].releaseFenceFd = release_fence1;
        contents[0]->hwLayers[1].releaseFenceFd = release_fence2;
        contents[0]->retireFenceFd = -1;
    };

    mg::RenderableList renderlist{
        stub_renderable1,
        stub_renderable2
    };

    layer.acquireFenceFd = acquire_fence1;
    layer2.acquireFenceFd = acquire_fence2;
    layer.compositionType = HWC_OVERLAY;
    layer2.compositionType = HWC_OVERLAY;
    std::list<hwc_layer_1_t*> expected_list1
    {
        &layer,
        &layer2,
        &target_layer
    };

    hwc_rect_t update_layer_rect;
    hwc_layer_1_t update_layer;
    hwc_rect_t nofence_rect;
    hwc_layer_1_t nofence_layer;
    mt::fill_hwc_layer(update_layer, &update_layer_rect, position1, *updated_buffer, HWC_OVERLAY, 0);
    mt::fill_hwc_layer(nofence_layer, &nofence_rect, position2, *stub_buffer2, HWC_OVERLAY, 0);
    update_layer.acquireFenceFd = acquire_fence3;
    nofence_layer.acquireFenceFd = -1;
    std::list<hwc_layer_1_t*> expected_list2
    {
        &update_layer,
        &nofence_layer,
        &target_layer
    };

    Sequence seq; 
    EXPECT_CALL(*mock_device, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke(set_all_layers_to_overlay));
    EXPECT_CALL(*mock_native_buffer1, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(acquire_fence1));
    EXPECT_CALL(*mock_native_buffer2, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(acquire_fence2));
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list1)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences_fn));
    EXPECT_CALL(*mock_device, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke(set_all_layers_to_overlay));
    EXPECT_CALL(*native_buffer, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(acquire_fence3));
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list2)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences_fn));

    mga::HwcDevice device(mock_device);
    mga::LayerList list(layer_adapter, renderlist, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    //set only the 2nd layer to a new buffer. the first buffer has the same buffer, and would 
    //still be onscreen if this wasn't against a mock
    stub_renderable1->set_buffer(updated_buffer);
    list.update_list(renderlist, geom::Displacement{});
    device.commit({content});
}

TEST_F(HwcDevice, does_not_own_framebuffer_buffers_past_set)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare(_))
       .WillOnce(Invoke([&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
        {
            ASSERT_THAT(contents[0]->numHwLayers, Ge(2));
            contents[0]->hwLayers[0].compositionType = HWC_FRAMEBUFFER;
            contents[0]->hwLayers[1].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));

    mga::HwcDevice device(mock_device);

    auto use_count_before = stub_buffer1.use_count();

    mga::LayerList list(layer_adapter, {stub_renderable1}, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    EXPECT_THAT(stub_buffer1.use_count(), Eq(use_count_before));
}

TEST_F(HwcDevice, rejects_empty_list)
{
    mga::HwcDevice device(mock_device);
    mg::RenderableList renderlist{};
    EXPECT_FALSE(device.compatible_renderlist(renderlist));
}

//TODO: we could accept a 90 degree transform
TEST_F(HwcDevice, rejects_list_containing_transformed)
{
    mga::HwcDevice device(mock_device);

    auto renderable = std::make_shared<mtd::StubTransformedRenderable>();
    mg::RenderableList renderlist{renderable};
    EXPECT_FALSE(device.compatible_renderlist(renderlist));
}

//TODO: support plane alpha for hwc 1.2 and later
TEST_F(HwcDevice, rejects_list_containing_plane_alpha)
{
    mga::HwcDevice device(mock_device);
    mg::RenderableList renderlist{std::make_shared<mtd::PlaneAlphaRenderable>()};
    EXPECT_FALSE(device.compatible_renderlist(renderlist));
}

TEST_F(HwcDevice, does_not_own_overlay_buffers_after_screen_off)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare(_))
        .WillOnce(Invoke(set_all_layers_to_overlay));

    mga::HwcDevice device(mock_device);

    auto use_count_before = stub_buffer1.use_count();
    mga::LayerList list(layer_adapter, {stub_renderable1}, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    EXPECT_THAT(stub_buffer1.use_count(), Gt(use_count_before));

    device.content_cleared();
    EXPECT_THAT(stub_buffer1.use_count(), Eq(use_count_before));
}

TEST_F(HwcDevice, tracks_hwc_owned_fences_even_across_list_changes)
{
    using namespace testing;
    hwc_layer_1_t layer3;
    mt::fill_hwc_layer(layer2, &comp_rect, position1, *stub_buffer1, HWC_FRAMEBUFFER, 0);
    mt::fill_hwc_layer(layer3, &comp2_rect, position2, *stub_buffer2, HWC_FRAMEBUFFER, 0);

    int acquire_fence1 = 39303;
    int acquire_fence2 = 393044;
    int release_fence1 = 39304;
    int release_fence2 = 123;
    int release_fence3 = 136;
    mg::RenderableList renderlist1{
        stub_renderable1
    };
    mg::RenderableList renderlist2{
        stub_renderable1,
        stub_renderable2
    };

    layer.acquireFenceFd = acquire_fence1;
    layer.compositionType = HWC_OVERLAY;
    std::list<hwc_layer_1_t*> expected_list1
    {
        &layer,
        &target_layer
    };
    auto set_fences = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
    {
        ASSERT_THAT(contents[0], testing::NotNull());
        ASSERT_EQ(contents[0]->numHwLayers, 2);
        contents[0]->hwLayers[0].releaseFenceFd = release_fence1;
        contents[0]->retireFenceFd = -1;
    };
    auto set_fences2 = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
    {
        ASSERT_THAT(contents[0], testing::NotNull());
        ASSERT_EQ(contents[0]->numHwLayers, 3);
        contents[0]->hwLayers[0].releaseFenceFd = release_fence2;
        contents[0]->hwLayers[1].releaseFenceFd = release_fence3;
        contents[0]->retireFenceFd = -1;
    };

    layer2.acquireFenceFd = -1;
    layer2.compositionType = HWC_OVERLAY;
    layer3.acquireFenceFd = acquire_fence2;
    layer3.compositionType = HWC_OVERLAY;
    std::list<hwc_layer_1_t*> expected_list2
    {
        &layer2,
        &layer3,
        &target_layer
    };

    Sequence seq;
    // first post 
    EXPECT_CALL(*mock_device, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke(set_all_layers_to_overlay));
    EXPECT_CALL(*mock_native_buffer1, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(acquire_fence1));
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list1)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences));
    EXPECT_CALL(*mock_native_buffer1, update_usage(release_fence1, mga::BufferAccess::read))
        .InSequence(seq);

    //end first post, second post
    EXPECT_CALL(*mock_device, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke(set_all_layers_to_overlay));
    //note that only the 2nd buffer should have its fence copied
    EXPECT_CALL(*mock_native_buffer2, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(acquire_fence2));
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list2)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences2));
    EXPECT_CALL(*mock_native_buffer1, update_usage(release_fence2, mga::BufferAccess::read))
        .InSequence(seq);
    EXPECT_CALL(*mock_native_buffer2, update_usage(release_fence3, mga::BufferAccess::read))
        .InSequence(seq);
    //end second post

    mga::HwcDevice device(mock_device);
    mga::LayerList list(layer_adapter, renderlist1, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    list.update_list(renderlist2, geom::Displacement{});
    device.commit({content});
}

TEST_F(HwcDevice, tracks_hwc_owned_fences_across_list_rearrange)
{
    using namespace testing;
    hwc_layer_1_t layer3;
    hwc_layer_1_t layer4;
    mt::fill_hwc_layer(layer3, &comp2_rect, position2, *stub_buffer2, HWC_FRAMEBUFFER, 0);
    mt::fill_hwc_layer(layer4, &comp_rect, position1, *stub_buffer1, HWC_FRAMEBUFFER, 0);

    int acquire_fence1 = 39303;
    int acquire_fence2 = 393044;
    int release_fence1 = 39304;
    int release_fence2 = 123;
    int release_fence3 = 136;
    int release_fence4 = 1344;
    mg::RenderableList renderlist{
        stub_renderable1,
        stub_renderable2
    };

    //the renderable ptr or the buffer ptr could change, while still referencing the same buffer_handle_t
    mg::RenderableList renderlist2{
        std::make_shared<mtd::StubRenderable>(
            std::make_shared<mtd::StubBuffer>(mock_native_buffer2, size2), position2),
        std::make_shared<mtd::StubRenderable>(
            std::make_shared<mtd::StubBuffer>(mock_native_buffer1, size1), position1),
    };

    layer.acquireFenceFd = acquire_fence1;
    layer.compositionType = HWC_OVERLAY;
    layer2.acquireFenceFd = acquire_fence2;
    layer2.compositionType = HWC_OVERLAY;
    std::list<hwc_layer_1_t*> expected_list1
    {
        &layer,
        &layer2,
        &target_layer
    };
    auto set_fences = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
    {
        ASSERT_EQ(contents[0]->numHwLayers, 3);
        contents[0]->hwLayers[0].releaseFenceFd = release_fence1;
        contents[0]->hwLayers[1].releaseFenceFd = release_fence2;
        contents[0]->retireFenceFd = -1;
    };
    auto set_fences2 = [&](std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& contents)
    {
        ASSERT_EQ(contents[0]->numHwLayers, 3);
        contents[0]->hwLayers[0].releaseFenceFd = release_fence3;
        contents[0]->hwLayers[1].releaseFenceFd = release_fence4;
        contents[0]->retireFenceFd = -1;
    };

    layer3.acquireFenceFd = -1;
    layer3.compositionType = HWC_OVERLAY;
    layer4.acquireFenceFd = -1;
    layer4.compositionType = HWC_OVERLAY;
    std::list<hwc_layer_1_t*> expected_list2
    {
        &layer3,
        &layer4,
        &target_layer
    };

    Sequence seq;
    // first post 
    EXPECT_CALL(*mock_device, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke(set_all_layers_to_overlay));
    EXPECT_CALL(*mock_native_buffer1, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(acquire_fence1));
    EXPECT_CALL(*mock_native_buffer2, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(acquire_fence2));
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list1)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences));
    EXPECT_CALL(*mock_native_buffer1, update_usage(release_fence1, mga::BufferAccess::read))
        .InSequence(seq);
    EXPECT_CALL(*mock_native_buffer2, update_usage(release_fence2, mga::BufferAccess::read))
        .InSequence(seq);

    //end first post, second post
    EXPECT_CALL(*mock_device, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke(set_all_layers_to_overlay));
    //note that the buffers just flipped position, no acquire fence copying.
    EXPECT_CALL(*mock_device, set(MatchesPrimaryList(expected_list2)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences2));
    EXPECT_CALL(*mock_native_buffer2, update_usage(release_fence3, mga::BufferAccess::read))
        .InSequence(seq);
    EXPECT_CALL(*mock_native_buffer1, update_usage(release_fence4, mga::BufferAccess::read))
        .InSequence(seq);
    //end second post

    mga::HwcDevice device(mock_device);
    mga::LayerList list(layer_adapter, renderlist, geom::Displacement{});
    mga::DisplayContents content{primary, list, offset, stub_context, stub_compositor};
    device.commit({content});
    list.update_list(renderlist2, geom::Displacement{});
    device.commit({content});
}

TEST_F(HwcDevice, commits_external_list_with_both_force_gl)
{
    using namespace testing;
    testing::NiceMock<mtd::MockSwappingGLContext> mock_context1;
    testing::NiceMock<mtd::MockSwappingGLContext> mock_context2;
    ON_CALL(mock_context1, last_rendered_buffer())
        .WillByDefault(Return(stub_fb_buffer));
    ON_CALL(mock_context2, last_rendered_buffer())
        .WillByDefault(Return(stub_fb_buffer));
    std::list<hwc_layer_1_t*> expected_list
    {
        &skip_layer,
        &target_layer
    };

    InSequence seq;
    EXPECT_CALL(*mock_device, prepare(MatchesLists(expected_list, expected_list)));
    EXPECT_CALL(mock_context1, make_current());
    EXPECT_CALL(mock_context1, release_current());
    EXPECT_CALL(mock_context2, make_current());
    EXPECT_CALL(mock_context2, release_current());
    EXPECT_CALL(*mock_device, set(MatchesLists(expected_list, expected_list)));

    mga::LayerList primary_list(layer_adapter, {}, geom::Displacement{});
    mga::LayerList external_list(layer_adapter, {}, geom::Displacement{});

    mga::HwcDevice device(mock_device);

    mga::DisplayContents primary_content{
        primary, primary_list, offset, mock_context1, stub_compositor};
    mga::DisplayContents external_content{
        mga::DisplayName::external, external_list, offset, mock_context2, stub_compositor};

    device.commit({primary_content, external_content});
}
