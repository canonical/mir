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

#include "mir/graphics/android/android_display.h"
#include "mir_test/egl_mock.h"
#include <gtest/gtest.h>
#include <memory>

namespace mg=mir::graphics;

TEST(framebuffer_test, fb_initialize)
{
    using namespace testing;

    mir::EglMock mock_egl;
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay);
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
            .Times(Exactly(1));
/*
    glClearColor(1.0f, 1.0f , 1.0f, 1.0f);
    glClearBit(); 
*/
    display->notify_update();
}
