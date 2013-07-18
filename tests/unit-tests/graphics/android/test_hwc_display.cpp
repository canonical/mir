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

#include "mir/graphics/display_buffer.h"
#include "src/server/graphics/android/android_display.h"
#include "src/server/graphics/android/hwc_android_display_buffer_factory.h"

#include "mir_test_doubles/mock_hwc_interface.h"
#include "mir_test_doubles/mock_android_framebuffer_window.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_egl.h"

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

    std::shared_ptr<mga::AndroidDisplay> create_display()
    {
        auto db_factory = std::make_shared<mga::HWCAndroidDisplayBufferFactory>(mock_hwc_device);
        return std::make_shared<mga::AndroidDisplay>(native_win, db_factory, mock_display_report);
    }

    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
    std::shared_ptr<mtd::MockAndroidFramebufferWindow> native_win;
    std::shared_ptr<mtd::MockHWCInterface> mock_hwc_device;

    mtd::MockEGL mock_egl;
};

TEST_F(AndroidTestHWCFramebuffer, test_post_submits_right_egl_parameters)
{
    using namespace testing;

    auto display = create_display();

    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_hwc_device, commit_frame(mock_egl.fake_egl_display, mock_egl.fake_egl_surface))
        .Times(1);

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.post_update();
    });
}

TEST_F(AndroidTestHWCFramebuffer, test_hwc_reports_size_correctly)
{
    using namespace testing;

    geom::Size fake_display_size{223, 332};
    EXPECT_CALL(*mock_hwc_device, display_size())
        .Times(1)
        .WillOnce(Return(fake_display_size)); 
    auto display = create_display();
    
    auto view_area = display->view_area();

    geom::Point origin_pt{geom::X{0}, geom::Y{0}};
    EXPECT_EQ(view_area.size, fake_display_size);
    EXPECT_EQ(view_area.top_left, origin_pt);
}
