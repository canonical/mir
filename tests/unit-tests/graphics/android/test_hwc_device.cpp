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

#include "mir/graphics/android/sync_fence.h"
#include "src/platform/graphics/android/framebuffer_bundle.h"
#include "src/platform/graphics/android/hwc_device.h"
#include "src/platform/graphics/android/hwc_layerlist.h"
#include "src/platform/graphics/android/gl_context.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/fake_shared.h"
#include "hwc_struct_helpers.h"
#include "mir_test_doubles/mock_render_function.h"
#include "mir_test_doubles/mock_swapping_gl_context.h"
#include "mir_test_doubles/stub_swapping_gl_context.h"
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

class StubRenderable : public mg::Renderable
{
public:
    StubRenderable(std::shared_ptr<mg::Buffer> const& buffer, geom::Rectangle screen_pos)
        : buf(buffer),
          screen_pos(screen_pos)
    {
    }

    std::shared_ptr<mg::Buffer> buffer(unsigned long) const
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

    float alpha() const
    {
        return 1.0;
    }
    
    glm::mat4 transformation() const
    {
        static glm::mat4 matrix;
        return matrix;
    }

    bool should_be_rendered_in(geom::Rectangle const&) const
    {
        return true;
    }

    bool shaped() const
    {
        return true;
    }

    int buffers_ready_for_compositor() const override
    {
        return 1;
    }
private:
    std::shared_ptr<mg::Buffer> buf;
    geom::Rectangle screen_pos;
};

struct MockFileOps : public mga::SyncFileOps
{
    MOCK_METHOD3(ioctl, int(int,int,void*));
    MOCK_METHOD1(dup, int(int));
    MOCK_METHOD1(close, int(int));
};

struct MockHWCDeviceWrapper : public mga::HwcWrapper
{
    MOCK_CONST_METHOD1(prepare, void(hwc_display_contents_1_t&));
    MOCK_CONST_METHOD1(set, void(hwc_display_contents_1_t&));
};
}

class HwcDevice : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        mock_native_buffer = std::make_shared<testing::NiceMock<mtd::MockAndroidNativeBuffer>>();
        mock_native_buffer->anwb()->width = buffer_size.width.as_int();
        mock_native_buffer->anwb()->height =  buffer_size.height.as_int();

        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();
        mock_file_ops = std::make_shared<MockFileOps>();

        ON_CALL(mock_buffer, size())
            .WillByDefault(Return(buffer_size));
        ON_CALL(mock_buffer, native_buffer_handle())
            .WillByDefault(Return(mock_native_buffer));

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
        comp_layer.handle = mock_native_buffer->handle();
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
        set_skip_layer.handle = mock_native_buffer->handle();
        set_skip_layer.sourceCrop = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()}; 
        set_skip_layer.displayFrame = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()};

        set_target_layer = target_layer;
        set_target_layer.handle = mock_native_buffer->handle();
        set_target_layer.sourceCrop = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()}; 
        set_target_layer.displayFrame = {0, 0, buffer_size.width.as_int(), buffer_size.height.as_int()};

        stub_renderable1 = std::make_shared<StubRenderable>(
                                mt::fake_shared(mock_buffer), screen_position);
        stub_renderable2 = std::make_shared<StubRenderable>(
                                mt::fake_shared(mock_buffer), screen_position);

        empty_prepare_fn = [] (hwc_display_contents_1_t&) {};
        empty_render_fn = [] (mg::Renderable const&) {};

        mock_hwc_device_wrapper = std::make_shared<testing::NiceMock<MockHWCDeviceWrapper>>();
    }

    std::shared_ptr<MockFileOps> mock_file_ops;
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;

    std::shared_ptr<mtd::MockAndroidNativeBuffer> mock_native_buffer;

    EGLDisplay dpy;
    EGLSurface surf;

    hwc_rect_t screen_pos;
    hwc_rect_t empty_region;
    hwc_rect_t set_region;
    hwc_layer_1_t skip_layer;
    hwc_layer_1_t target_layer;
    hwc_layer_1_t set_skip_layer;
    hwc_layer_1_t set_target_layer;
    hwc_layer_1_t comp_layer;

    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},{245, 250}};
    std::shared_ptr<StubRenderable> stub_renderable1;
    std::shared_ptr<StubRenderable> stub_renderable2;
    std::shared_ptr<MockHWCDeviceWrapper> mock_hwc_device_wrapper;

    std::function<void(hwc_display_contents_1_t&)> empty_prepare_fn;
    std::function<void(mg::Renderable const&)> empty_render_fn;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;
    mtd::MockSwappingGLContext mock_context;
    mtd::StubSwappingGLContext stub_context;
};


TEST_F(HwcDevice, prepares_a_skip_and_target_layer_by_default)
{
    using namespace testing;
    std::list<hwc_layer_1_t*> expected_list
    {
        &skip_layer,
        &target_layer
    };

    EXPECT_CALL(*mock_hwc_device_wrapper, prepare(MatchesList(expected_list)))
        .Times(1);

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.render_gl(stub_context);
}

TEST_F(HwcDevice, calls_render_fn_and_swap_when_all_overlays_are_rejected)
{
    using namespace testing;
    mtd::MockRenderFunction mock_render_fn;
    auto render_fn = [&](mg::Renderable const& renderable)
    {
        mock_render_fn.called(renderable);
    };

    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    std::list<hwc_layer_1_t*> expected_list
    {
        &comp_layer,
        &comp_layer,
        &target_layer
    };

    Sequence seq;
    EXPECT_CALL(*mock_hwc_device_wrapper, prepare(MatchesList(expected_list)))
        .InSequence(seq)
        .WillOnce(Invoke([&](hwc_display_contents_1_t& contents)
        {
            ASSERT_EQ(contents.numHwLayers, 3);
            contents.hwLayers[0].compositionType = HWC_FRAMEBUFFER;
            contents.hwLayers[1].compositionType = HWC_FRAMEBUFFER;
            contents.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));
    EXPECT_CALL(mock_render_fn, called(Ref(*stub_renderable1)))
        .InSequence(seq);
    EXPECT_CALL(mock_render_fn, called(Ref(*stub_renderable2)))
        .InSequence(seq);
    EXPECT_CALL(mock_context, swap_buffers())
        .InSequence(seq);

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.render_gl_and_overlays(mock_context, updated_list, render_fn);
}

TEST_F(HwcDevice, calls_render_and_swap_when_some_overlays_are_rejected)
{
    using namespace testing;
    mtd::MockRenderFunction mock_render_fn;
    auto render_fn = [&](mg::Renderable const& renderable)
    {
        mock_render_fn.called(renderable);
    };

    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    std::list<hwc_layer_1_t*> expected_list
    {
        &comp_layer,
        &comp_layer,
        &target_layer
    };

    Sequence seq;
    EXPECT_CALL(*mock_hwc_device_wrapper, prepare(MatchesList(expected_list)))
        .InSequence(seq)
        .WillOnce(Invoke([&](hwc_display_contents_1_t& contents)
        {
            ASSERT_EQ(contents.numHwLayers, 3);
            contents.hwLayers[0].compositionType = HWC_OVERLAY;
            contents.hwLayers[1].compositionType = HWC_FRAMEBUFFER;
            contents.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));
    EXPECT_CALL(mock_render_fn, called(Ref(*stub_renderable2)))
        .InSequence(seq);
    EXPECT_CALL(mock_context, swap_buffers())
        .InSequence(seq);

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.render_gl_and_overlays(mock_context, updated_list, render_fn);
}

TEST_F(HwcDevice, does_not_call_render_or_swap_when_all_overlays_accepted)
{
    using namespace testing;
    mtd::MockRenderFunction mock_render_fn;
    auto render_fn = [&](mg::Renderable const& renderable)
    {
        mock_render_fn.called(renderable);
    };

    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    std::list<hwc_layer_1_t*> expected_list
    {
        &comp_layer,
        &comp_layer,
        &target_layer
    };

    EXPECT_CALL(*mock_hwc_device_wrapper, prepare(MatchesList(expected_list)))
        .Times(1)
        .WillOnce(Invoke([&](hwc_display_contents_1_t& contents)
        {
            ASSERT_EQ(contents.numHwLayers, 3);
            contents.hwLayers[0].compositionType = HWC_OVERLAY;
            contents.hwLayers[1].compositionType = HWC_OVERLAY;
            contents.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));
    EXPECT_CALL(mock_render_fn, called(_))
        .Times(0);
    EXPECT_CALL(mock_context, swap_buffers())
        .Times(0);

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.render_gl_and_overlays(mock_context, updated_list, render_fn);
}

TEST_F(HwcDevice, resets_layers_when_prepare_gl_called)
{
    using namespace testing;
    std::list<hwc_layer_1_t*> expected_list1
    {
        &comp_layer,
        &comp_layer,
        &target_layer
    };

    std::list<hwc_layer_1_t*> expected_list2
    {
        &skip_layer,
        &target_layer
    };

    Sequence seq;
    EXPECT_CALL(*mock_hwc_device_wrapper, prepare(MatchesList(expected_list1)))
        .InSequence(seq);
    EXPECT_CALL(*mock_hwc_device_wrapper, prepare(MatchesList(expected_list2)))
        .InSequence(seq);
    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);

    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    device.render_gl_and_overlays(stub_context, updated_list, [](mg::Renderable const&){});
    device.render_gl(stub_context);
}

TEST_F(HwcDevice, sets_and_updates_fences)
{
    using namespace testing;
    int fb_release_fence = 94;
    int hwc_retire_fence = 74;
    auto set_fences_fn = [&](hwc_display_contents_1_t& contents)
    {
        ASSERT_EQ(contents.numHwLayers, 2);
        contents.hwLayers[1].releaseFenceFd = fb_release_fence;
        contents.retireFenceFd = hwc_retire_fence;
    };

    std::list<hwc_layer_1_t*> expected_list
    {
        &set_skip_layer,
        &set_target_layer
    };

    Sequence seq;
    EXPECT_CALL(*mock_hwc_device_wrapper, set(MatchesList(expected_list)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences_fn));
    EXPECT_CALL(*mock_native_buffer, update_fence(fb_release_fence))
        .InSequence(seq);
    EXPECT_CALL(*mock_file_ops, close(hwc_retire_fence))
        .InSequence(seq);

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.render_gl(stub_context);
    device.post(mock_buffer);
}

TEST_F(HwcDevice, sets_proper_list_with_overlays)
{
    using namespace testing;
    int overlay_acquire_fence1 = 80;
    int overlay_acquire_fence2 = 81;
    int fb_acquire_fence = 82;
    int release_fence1 = 381;
    int release_fence2 = 382;
    int release_fence3 = 383;
    auto native_handle_1 = std::make_shared<mtd::StubAndroidNativeBuffer>();
    auto native_handle_2 = std::make_shared<mtd::StubAndroidNativeBuffer>();
    auto native_handle_3 = std::make_shared<mtd::StubAndroidNativeBuffer>();
    native_handle_1->anwb()->width = buffer_size.width.as_int();
    native_handle_1->anwb()->height = buffer_size.height.as_int();
    native_handle_2->anwb()->width = buffer_size.width.as_int();
    native_handle_2->anwb()->height = buffer_size.height.as_int();
    native_handle_3->anwb()->width = buffer_size.width.as_int();
    native_handle_3->anwb()->height = buffer_size.height.as_int();

    EXPECT_CALL(mock_buffer, native_buffer_handle())
        .WillOnce(Return(native_handle_1))
        .WillOnce(Return(native_handle_2))
        .WillRepeatedly(Return(native_handle_3));

    auto set_fences_fn = [&](hwc_display_contents_1_t& contents)
    {
        ASSERT_EQ(contents.numHwLayers, 3);
        contents.hwLayers[0].releaseFenceFd = release_fence1;
        contents.hwLayers[1].releaseFenceFd = release_fence2;
        contents.hwLayers[2].releaseFenceFd = release_fence3;
        contents.retireFenceFd = -1;
    };

    /* set non-default renderlist */
    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable1
    });

    hwc_layer_1_t comp_layer1, comp_layer2;
    comp_layer1.compositionType = HWC_OVERLAY;
    comp_layer1.hints = 0;
    comp_layer1.flags = 0;
    comp_layer1.handle = native_handle_1->handle();
    comp_layer1.transform = 0;
    comp_layer1.blending = HWC_BLENDING_NONE;
    comp_layer1.sourceCrop = set_region;
    comp_layer1.displayFrame = screen_pos;
    comp_layer1.visibleRegionScreen = {1, &set_region};
    comp_layer1.acquireFenceFd = overlay_acquire_fence1;
    comp_layer1.releaseFenceFd = -1;

    comp_layer2.compositionType = HWC_OVERLAY;
    comp_layer2.hints = 0;
    comp_layer2.flags = 0;
    comp_layer2.handle = native_handle_2->handle();
    comp_layer2.transform = 0;
    comp_layer2.blending = HWC_BLENDING_NONE;
    comp_layer2.sourceCrop = set_region;
    comp_layer2.displayFrame = screen_pos;
    comp_layer2.visibleRegionScreen = {1, &set_region};
    comp_layer2.acquireFenceFd = overlay_acquire_fence2;
    comp_layer2.releaseFenceFd = -1;

    set_target_layer.acquireFenceFd = fb_acquire_fence;
    set_target_layer.handle = native_handle_3->handle();

    std::list<hwc_layer_1_t*> expected_list
    {
        &comp_layer1,
        &comp_layer2,
        &set_target_layer
    };

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);

    //all accepted
    Sequence seq; 
    EXPECT_CALL(*mock_hwc_device_wrapper, prepare(_))
        .InSequence(seq)
        .WillOnce(Invoke([&](hwc_display_contents_1_t& contents)
        {
            ASSERT_EQ(contents.numHwLayers, 3);
            contents.hwLayers[0].compositionType = HWC_OVERLAY;
            contents.hwLayers[1].compositionType = HWC_OVERLAY;
            contents.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));

    //copy all fb fences for OVERLAY or FRAMEBUFFER_TARGET in preparation of set
    EXPECT_CALL(*native_handle_1, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(overlay_acquire_fence1));
    EXPECT_CALL(*native_handle_2, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(overlay_acquire_fence2));
    EXPECT_CALL(*native_handle_3, copy_fence())
        .InSequence(seq)
        .WillOnce(Return(fb_acquire_fence));

    //set
    EXPECT_CALL(*mock_hwc_device_wrapper, set(MatchesList(expected_list)))
        .InSequence(seq)
        .WillOnce(Invoke(set_fences_fn));
    EXPECT_CALL(*native_handle_1, update_fence(release_fence1))
        .InSequence(seq);
    EXPECT_CALL(*native_handle_2, update_fence(release_fence2))
        .InSequence(seq);
    EXPECT_CALL(*native_handle_3, update_fence(release_fence3))
        .InSequence(seq);

    device.render_gl_and_overlays(stub_context, updated_list, [](mg::Renderable const&){});
    device.post(mock_buffer);
}

TEST_F(HwcDevice, discards_second_set_if_all_overlays_and_nothing_has_changed)
{
    using namespace testing;
    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);

    ON_CALL(*mock_hwc_device_wrapper, prepare(_))
        .WillByDefault(Invoke([&](hwc_display_contents_1_t& contents)
        {
            ASSERT_EQ(contents.numHwLayers, 3);
            contents.hwLayers[0].compositionType = HWC_OVERLAY;
            contents.hwLayers[1].compositionType = HWC_OVERLAY;
            contents.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));

    EXPECT_CALL(*mock_hwc_device_wrapper, set(_))
        .Times(1);

    device.render_gl_and_overlays(stub_context, updated_list, [](mg::Renderable const&){});
    device.post(mock_buffer);
    device.render_gl_and_overlays(stub_context, updated_list, [](mg::Renderable const&){});
    device.post(mock_buffer);
}

TEST_F(HwcDevice, submits_every_time_if_at_least_one_layer_is_gl_rendered)
{
    using namespace testing;
    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);

    ON_CALL(*mock_hwc_device_wrapper, prepare(_))
        .WillByDefault(Invoke([&](hwc_display_contents_1_t& contents)
        {
            ASSERT_EQ(contents.numHwLayers, 3);
            contents.hwLayers[0].compositionType = HWC_OVERLAY;
            contents.hwLayers[1].compositionType = HWC_FRAMEBUFFER;
            contents.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
        }));

    EXPECT_CALL(*mock_hwc_device_wrapper, set(_))
        .Times(2);

    device.render_gl_and_overlays(stub_context, updated_list, [](mg::Renderable const&){});
    device.post(mock_buffer);
    device.render_gl_and_overlays(stub_context, updated_list, [](mg::Renderable const&){});
    device.post(mock_buffer);
}
