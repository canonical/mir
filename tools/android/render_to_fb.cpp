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

#include "mir/graphics/android/android_framebuffer_window.h" 
#include "mir/graphics/android/android_display.h"

#include <ui/FramebufferNativeWindow.h>
#include <GLES2/gl2.h>

namespace mga=mir::graphics::android;

int main(int, char**)
{
    std::shared_ptr<ANativeWindow> android_window((ANativeWindow*) new android::FramebufferNativeWindow);
    std::shared_ptr<mga::AndroidFramebufferWindow> window (new mga::AndroidFramebufferWindow(android_window));
    std::shared_ptr<mga::AndroidDisplay> display(new mga::AndroidDisplay(window));

    glClearColor(0.0, 1.0, 0.0, 1.0);

    for(;;)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        display->post_update();
    }

    return 0;
}

