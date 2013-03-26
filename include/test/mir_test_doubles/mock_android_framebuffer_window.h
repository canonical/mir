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

#ifndef MIR_TEST_DOUBLES_MOCK_ANDROID_FRAMEBUFFER_WINDOW_H_
#define MIR_TEST_DOUBLES_MOCK_ANDROID_FRAMEBUFFER_WINDOW_H_

#include "src/server/graphics/android/android_framebuffer_window_query.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockAndroidFramebufferWindow : public mir::graphics::android::AndroidFramebufferWindowQuery
{
public:
    MockAndroidFramebufferWindow() {}
    ~MockAndroidFramebufferWindow() {}

    MOCK_CONST_METHOD0(android_native_window_type, EGLNativeWindowType());
    MOCK_CONST_METHOD1(android_display_egl_config, EGLConfig(EGLDisplay));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_ANDROID_FRAMEBUFFER_WINDOW_H_ */
