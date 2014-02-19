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
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_hwc_vsync_coordinator.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/fake_shared.h"
#include "hwc_struct_helpers.h"
#include "mir_test_doubles/mock_render_function.h"
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

        dpy = mock_egl.fake_egl_display;
        surf = mock_egl.fake_egl_surface;
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

        mock_hwc_device_wrapper = std::make_shared<MockHWCDeviceWrapper>();
    }

    std::shared_ptr<MockFileOps> mock_file_ops;
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;

    std::shared_ptr<mtd::MockAndroidNativeBuffer> mock_native_buffer;

    EGLDisplay dpy;
    EGLSurface surf;
    testing::NiceMock<mtd::MockEGL> mock_egl;

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
    testing::NiceMock<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<StubRenderable> stub_renderable1;
    std::shared_ptr<StubRenderable> stub_renderable2;
    std::shared_ptr<MockHWCDeviceWrapper> mock_hwc_device_wrapper;

    std::function<void(hwc_display_contents_1_t&)> empty_prepare_fn;
    std::function<void(mg::Renderable const&)> empty_render_fn;
};


#if 0
TEST_F(HwcDevice, hwc_displays)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(),_,_))
        .Times(1);
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(),_,_))
        .Times(1);

    mga::HwcDeviceWrapper device(mock_device);

    hwc_display_contents_1_t contents;
    device.prepare(contents);
    device.set(contents);

    /* primary phone display */
    EXPECT_TRUE(mock_device->primary_prepare);
    EXPECT_TRUE(mock_device->primary_set);
    /* external monitor display not supported yet */
    EXPECT_FALSE(mock_device->external_prepare);
    EXPECT_FALSE(mock_device->external_set);
    /* virtual monitor display not supported yet */
    EXPECT_FALSE(mock_device->virtual_prepare);
    EXPECT_FALSE(mock_device->virtual_set);
}
#endif

TEST_F(HwcDevice, hwc_prepare)
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
    device.prepare_gl();
}

TEST_F(HwcDevice, hwc_default_set)
{
    using namespace testing;
    int skip_release_fence = -1;
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
    EXPECT_CALL(*mock_native_buffer, update_fence(skip_release_fence))
        .InSequence(seq);
    EXPECT_CALL(*mock_native_buffer, update_fence(fb_release_fence))
        .InSequence(seq);
    EXPECT_CALL(*mock_file_ops, close(hwc_retire_fence))
        .InSequence(seq);

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.post(mock_buffer);
}

TEST_F(HwcDevice, hwc_prepare_with_overlays_all_rejected)
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

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.prepare_gl_and_overlays(updated_list, render_fn);
}

TEST_F(HwcDevice, hwc_prepare_with_overlays_some_rejected)
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

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.prepare_gl_and_overlays(updated_list, render_fn);
}

TEST_F(HwcDevice, hwc_prepare_with_overlays_all_accepted)
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

    mga::HwcDevice device(mock_device, mock_hwc_device_wrapper, mock_vsync, mock_file_ops);
    device.prepare_gl_and_overlays(updated_list, render_fn);
}

#if 0

TEST_F(HwcDevice, hwc_swapbuffers)
{
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1);
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(), 1, _))
        .Times(1)
        .WillOnce(Invoke(prepare_fn_all_gl));

    mga::HwcDevice device(mock_device, mock_vsync, mock_file_ops);

    device.gpu_render(dpy, surf);
}

TEST_F(HwcDevice, hwc_swapbuffers_skipped_if_not_needed)
{
    auto prepare_fn_all_overlay = [] (hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(3, list.numHwLayers);
        list.hwLayers[0].compositionType = HWC_OVERLAY;
        list.hwLayers[1].compositionType = HWC_OVERLAY;
        list.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
    };
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(), 1, _))
        .Times(1)
        .WillOnce(Invoke(prepare_fn_all_overlay));
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(0);
    mga::HwcDevice device(mock_device, mock_vsync, mock_file_ops);
    device.gpu_render(dpy, surf);
}

TEST_F(HwcDevice, hwc_swapbuffers_failure)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglSwapBuffers(dpy,surf))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    mga::HwcDevice device(mock_device, mock_vsync, mock_file_ops);

    EXPECT_THROW({
        device.gpu_render(dpy, surf);
    }, std::runtime_error);
}
#endif 

#if 0
//to hwc device adaptor
TEST_F(HwcDevice, hwc_commit_failure)
{
    using namespace testing;

    mga::HwcDevice device(mock_device, mock_vsync, mock_file_ops);

    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), _, _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        device.post(*mock_buffer);
    }, std::runtime_error);
}
#endif



/* tests with a FRAMEBUFFER_TARGET present
   NOT PORTEDDDDDDDDDDDDDDDDDDDd */
#if 0
TEST_F(HwcDevice, hwc_prepare_resets_layers)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(), 1, _))
        .Times(2);

    mga::HwcDevice device(mock_device, mock_vsync, mock_file_ops);

    std::list<std::shared_ptr<mg::Renderable>> renderlist
    {
        std::make_shared<mtd::MockRenderable>(),
        std::make_shared<mtd::MockRenderable>()
    };
    device.prepare_gl_and_overlays(renderlist);
    EXPECT_EQ(3, mock_device->display0_prepare_content.numHwLayers);

    device.prepare_gl();
    EXPECT_EQ(2, mock_device->display0_prepare_content.numHwLayers);
}
#endif



#if 0
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
    EXPECT_TRUE(layerlist.prepare_default_layers(empty_prepare_fn));

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

#endif
