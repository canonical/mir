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

#include "mir_test_doubles/mock_android_framebuffer_window.h"

class AndroidTestHWCFramebuffer : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        native_win = std::make_shared<mtd::MockAndroidFramebufferWindow>();

        /* silence uninteresting warning messages */
        mock_egl.silence_uninteresting();
    }

    std::shared_ptr<mtd::MockAndroidFramebufferWindow> native_win;

    mir::EglMock mock_egl;
    mt::HardwareAccessMock hw_access_mock;
};

TEST_F(AndroidTestHWCFramebuffer, test_construction_registers_procs)
{
    EXPECT_CALL(hw_access_mock, registerProc())
        .Times(1)
        .WillOnce(DoAll(SaveArg<X>(&procs), Return(1)));

    mga::AndroidHWCDisplay display;

    test::VerifyAndClear;

     EXPECT_NE(nullptr, proc->vsync)
     EXPECT_NE(nullptr, proc->hotplug)
     EXPECT_NE(nullptr, proc->otherthing)
}

#if 0
TEST_F(AndroidTestHWCFramebuffer, test_construction_activates_vsync_signal)
{
    EXPECT_CALL(hw_access_mock, activate_vsync())
        .Times(1);

    mga::AndroidHWCDisplay display;

}

TEST_F(AndroidTestHWCFramebuffer, test_vsync_signal_wait)
{

}

TEST_F(AndroidTestHWCFramebuffer, test_unconsumed_vsync_signal_wait)
{

}
#endif
