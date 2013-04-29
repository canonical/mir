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

#include "src/server/graphics/android/hwc_display.h"

#include "mir_test_doubles/mock_hwc_interface.h"
#include "mir_test_doubles/mock_android_framebuffer_window.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test/egl_mock.h"

#include <memory>

namespace geom=mir::geometry;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

class AndroidTestHWCFramebuffer : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        native_win = std::make_shared<testing::NiceMock<mtd::MockAndroidFramebufferWindow>>();
        mock_hwc_device = std::make_shared<mtd::MockHWCInterface>();

        /* silence uninteresting warning messages */
        mock_egl.silence_uninteresting();
        mock_display_report = std::make_shared<mtd::MockDisplayReport>();
    }

    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
    std::shared_ptr<mtd::MockAndroidFramebufferWindow> native_win;
    std::shared_ptr<mtd::MockHWCInterface> mock_hwc_device;

    mir::EglMock mock_egl;
};

TEST_F(AndroidTestHWCFramebuffer, test_post_submits_right_egl_parameters)
{
    using namespace testing;

    mga::HWCDisplay display(native_win, mock_hwc_device, mock_display_report);

    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_hwc_device, commit_frame(mock_egl.fake_egl_display, mock_egl.fake_egl_surface))
        .Times(1);
    EXPECT_CALL(*mock_hwc_device, wait_for_vsync())
        .Times(1);

    display.for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.post_update();
    });
}

TEST_F(AndroidTestHWCFramebuffer, test_vsync_signal_wait_on_post)
{
    using namespace testing;

    mga::HWCDisplay display(native_win, mock_hwc_device, mock_display_report);

    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_hwc_device, commit_frame(_,_))
        .Times(1);
    EXPECT_CALL(*mock_hwc_device, wait_for_vsync())
        .Times(1);

    display.for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.post_update();
    });
}

TEST_F(AndroidTestHWCFramebuffer, test_hwc_reports_size_correctly)
{
    using namespace testing;

    geom::Size fake_display_size{geom::Width{223}, geom::Height{332}};
    EXPECT_CALL(*mock_hwc_device, display_size())
        .Times(1)
        .WillOnce(Return(fake_display_size)); 
    mga::HWCDisplay display(native_win, mock_hwc_device, mock_display_report);
    
    auto view_area = display.view_area();

    geom::Point origin_pt{geom::X{0}, geom::Y{0}};
    EXPECT_EQ(view_area.size, fake_display_size);
    EXPECT_EQ(view_area.top_left, origin_pt);
}
