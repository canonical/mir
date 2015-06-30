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

#include "mir/test/doubles/mock_fb_hal_device.h"
#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/stub_android_native_buffer.h"
#include "src/platforms/android/server/fb_device.h"
#include "src/platforms/android/server/hwc_fallback_gl_renderer.h"
#include "src/platforms/android/server/hwc_layerlist.h"
#include "mir/test/doubles/mock_framebuffer_bundle.h"
#include "mir/test/doubles/mock_android_hw.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir/test/doubles/mock_swapping_gl_context.h"
#include "mir/test/doubles/stub_renderable_list_compositor.h"
#include "mir/test/doubles/mock_renderable_list_compositor.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;
namespace mt=mir::test;

struct FBDevice : public ::testing::Test
{
    virtual void SetUp()
    {
        using namespace testing;

        fbnum = 4;
        format = HAL_PIXEL_FORMAT_RGBA_8888;

        fb_hal_mock = std::make_shared<NiceMock<mtd::MockFBHalDevice>>(
            display_size.width.as_int(), display_size.height.as_int(), format, fbnum);
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>();
        native_buffer = std::make_shared<mtd::StubAndroidNativeBuffer>();
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_buffer));
        ON_CALL(mock_context, last_rendered_buffer())
            .WillByDefault(Return(mock_buffer));
    }

    unsigned int format, fbnum;
    geom::Size display_size{413, 516};
    std::shared_ptr<mtd::MockFBHalDevice> fb_hal_mock;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<mir::graphics::NativeBuffer> native_buffer;
    mtd::HardwareAccessMock hw_access_mock;
    testing::NiceMock<mtd::MockSwappingGLContext> mock_context;
    mga::LayerList list{std::make_shared<mga::IntegerSourceCrop>(), {}, geom::Displacement{}};
    mtd::StubRenderableListCompositor stub_compositor;
    mga::DisplayName primary{mga::DisplayName::primary};
};

TEST_F(FBDevice, rejects_renderables)
{
    mg::RenderableList renderlist
    {
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>()
    };

    mga::FBDevice fbdev(fb_hal_mock);
    EXPECT_FALSE(fbdev.compatible_renderlist(renderlist));
}

TEST_F(FBDevice, commits_frame)
{
    using namespace testing;
    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(), native_buffer->handle()))
        .Times(2)
        .WillOnce(Return(-1))
        .WillOnce(Return(0));

    mga::FBDevice fbdev(fb_hal_mock);
    mga::DisplayContents content{primary, list, geom::Displacement{}, mock_context, stub_compositor};

    EXPECT_THROW({
        fbdev.commit({content});
    }, std::runtime_error);

    fbdev.commit({content});

    // Predictive bypass not enabled in FBDevice
    EXPECT_EQ(0, fbdev.recommended_sleep().count());
}

//not all fb devices provide a swap interval hook. make sure we don't explode if thats the case
TEST_F(FBDevice, does_not_segfault_if_null_swapinterval_hook)
{
    fb_hal_mock->setSwapInterval = nullptr;
    mga::FbControl fb_control(fb_hal_mock);
}

TEST_F(FBDevice, can_screen_on_off)
{
    using namespace testing;
    Sequence seq;
    EXPECT_CALL(*fb_hal_mock, setSwapInterval_interface(fb_hal_mock.get(), 1))
        .Times(1);
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,0))
        .InSequence(seq);
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,0))
        .InSequence(seq);
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,0))
        .InSequence(seq);
    EXPECT_CALL(*fb_hal_mock, enableScreen_interface(_,1))
        .InSequence(seq);
 
    mga::FbControl fb_control(fb_hal_mock);
    fb_control.power_mode(mga::DisplayName::primary, mir_power_mode_standby);
    fb_control.power_mode(mga::DisplayName::primary, mir_power_mode_suspend);
    fb_control.power_mode(mga::DisplayName::primary, mir_power_mode_off);
    fb_control.power_mode(mga::DisplayName::primary, mir_power_mode_on);

    EXPECT_THROW({
        fb_control.power_mode(mga::DisplayName::external, mir_power_mode_on);
    }, std::runtime_error);
}

TEST_F(FBDevice, bundle_from_fb)
{
    using namespace testing;
    mga::FbControl fb_control(fb_hal_mock);
    auto attribs = fb_control.active_config_for(mga::DisplayName::primary);
    EXPECT_EQ(display_size, attribs.modes[attribs.current_mode_index].size);
    EXPECT_EQ(mir_pixel_format_abgr_8888, attribs.current_format);
}
