/*
 * Copyright © 2012 Canonical Ltd.
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
#include "mir_test/egl_mock.h"

#include <memory>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

class AndroidTestHWCFramebuffer : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        native_win = std::make_shared<mtd::MockAndroidFramebufferWindow>();
        mock_hwc_device = std::make_shared<mtd::MockHWCInterface>();

        /* silence uninteresting warning messages */
        mock_egl.silence_uninteresting();
    }

    std::shared_ptr<mtd::MockAndroidFramebufferWindow> native_win;
    std::shared_ptr<mtd::MockHWCInterface> mock_hwc_device;

    mir::EglMock mock_egl;
};

TEST_F(AndroidTestHWCFramebuffer, test_vsync_signal_wait)
{
    using namespace testing;

    mga::HWCDisplay display(native_win, mock_hwc_device);

    testing::InSequence sequence_enforcer;
    EXPECT_CALL(mock_egl, eglSwapBuffers(_,_))
        .Times(1);
    EXPECT_CALL(*mock_hwc_device, wait_for_vsync())
        .Times(1);
 
    display.post_update();
}

#if 0
//put in the hwc adapter class
TEST_F(AndroidTestHWCFramebuffer, test_construction_registers_procs)
{
    EXPECT_CALL(hw_access_mock, registerProc())
        .Times(1)
        .WillOnce(DoAll(SaveArg<X>(&procs), Return(1)));

    mga::AndroidHWCDisplay display(native_win, hwc_device);

    test::VerifyAndClear;

     EXPECT_NE(nullptr, proc->vsync)
     EXPECT_NE(nullptr, proc->hotplug)
     EXPECT_NE(nullptr, proc->otherthing)
}

TEST_F(AndroidTestHWCFramebuffer, test_construction_activates_vsync_signal)
{
    EXPECT_CALL(hw_access_mock, activate_vsync())
        .Times(1);

    mga::AndroidHWCDisplay display;

}


TEST_F(AndroidTestHWCFramebuffer, test_unconsumed_vsync_signal_wait)
{

}
#endif
