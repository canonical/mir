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
#include "mir_test_doubles/mock_renderable.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/stub_buffer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
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

struct MockFileOps : public mga::SyncFileOps
{
    MOCK_METHOD3(ioctl, int(int,int,void*));
    MOCK_METHOD1(dup, int(int));
    MOCK_METHOD1(close, int(int));
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
        mock_buffer = std::make_shared<testing::NiceMock<mtd::MockBuffer>>();
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_vsync = std::make_shared<testing::NiceMock<mtd::MockVsyncCoordinator>>();
        mock_file_ops = std::make_shared<MockFileOps>();

        ON_CALL(*mock_buffer, size())
            .WillByDefault(Return(geom::Size{0,0}));
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(mock_native_buffer));
    }

    std::shared_ptr<MockFileOps> mock_file_ops;
    std::shared_ptr<mtd::MockVsyncCoordinator> mock_vsync;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> mock_native_buffer;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    EGLDisplay dpy;
    EGLSurface surf;
    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(HwcDevice, hwc_displays)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(),_,_))
        .Times(1);
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(),_,_))
        .Times(1);

    mga::HwcDevice device(mock_device, mock_vsync, mock_file_ops);
    device.prepare_gl();
    device.post(*mock_buffer);

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

TEST_F(HwcDevice, hwc_prepare)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(), 1, _))
        .Times(1);

    mga::HwcDevice device(mock_device, mock_vsync, mock_file_ops);
    device.prepare_gl();

    ASSERT_EQ(2, mock_device->display0_prepare_content.numHwLayers);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
    EXPECT_EQ(-1, mock_device->display0_prepare_content.retireFenceFd);
}

TEST_F(HwcDevice, hwc_default_set)
{
    using namespace testing;
    int hwc_return_fence = 94;
    int hwc_retire_fence = 74;
    mock_device->hwc_set_return_fence(hwc_return_fence);
    mock_device->hwc_set_retire_fence(hwc_retire_fence);

    InSequence seq;
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), 1, _))
        .Times(1);
    EXPECT_CALL(*mock_native_buffer, update_fence(_))
        .Times(1);
    EXPECT_CALL(*mock_native_buffer, update_fence(hwc_return_fence))
        .Times(1);
    EXPECT_CALL(*mock_file_ops, close(hwc_retire_fence))
        .Times(1);

    device.post(*mock_buffer);

    ASSERT_EQ(2, mock_device->display0_set_content.numHwLayers);
    EXPECT_THAT(set_skip_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(set_target_layer, MatchesLayer(list->hwLayers[1]));
    EXPECT_EQ(-1, mock_device->display0_set_content.retireFenceFd);
    EXPECT_EQ(2, mock_device->display0_set_content.numHwLayers);
    EXPECT_EQ(-1, mock_device->display0_set_content.retireFenceFd);
    EXPECT_EQ(HWC_FRAMEBUFFER, mock_device->set_layerlist[0].compositionType);
    EXPECT_EQ(HWC_SKIP_LAYER, mock_device->set_layerlist[0].flags);
    EXPECT_EQ(HWC_FRAMEBUFFER_TARGET, mock_device->set_layerlist[1].compositionType);
    EXPECT_EQ(0, mock_device->set_layerlist[1].flags);
}

TEST_F(HwcDevice, hwc_prepare_with_overlays)
{
    using namespace testing;
    auto prepare_fn_all_gl = [] (hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(3, list.numHwLayers);
        list.hwLayers[0].compositionType = HWC_FRAMEBUFFER;
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

    auto prepare_fn_mixed = [] (hwc_display_contents_1_t& list)
    {
        ASSERT_EQ(3, list.numHwLayers);
        list.hwLayers[0].compositionType = HWC_OVERLAY;
        list.hwLayers[1].compositionType = HWC_FRAMEBUFFER;
        list.hwLayers[2].compositionType = HWC_FRAMEBUFFER_TARGET;
    };

    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(), 1, _))
        .Times(3)
        .WillOnce(Invoke(prepare_fn_all_gl))
        .WillOnce(Invoke(prepare_fn_all_overlay))
        .WillOnce(Invoke(prepare_fn_mixed))

    mga::HwcDevice device(mock_device, mock_vsync, mock_file_ops);
    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        stub_renderable1,
        stub_renderable2
    });

    auto call_count = 0;
    auto render_fn = [&call_count, this] (mg::Renderable const& renderable)
    {
        if (call_count == 0)
            EXPECT_EQ(&renderable, stub_renderable1.get());
        if (call_count == 1)
            EXPECT_EQ(&renderable, stub_renderable2.get());
        call_count++;
    };

    call_count = 0;
    layerlist.prepare_composition_layers(updated_list, render_fn);
    EXPECT_EQ(call_count, 2);

    call_count = 1;
    layerlist.prepare_composition_layers(updated_list, render_fn);
    EXPECT_EQ(call_count, 2);

    call_count = 0;
    layerlist.prepare_composition_layers(updated_list, render_fn);
    EXPECT_EQ(call_count, 0);
}

TEST_F(HWCLayerListTest, list_prepares_and_renders)
{

}
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

/* tests with a FRAMEBUFFER_TARGET present
   NOT PORTEDDDDDDDDDDDDDDDDDDDd */
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
