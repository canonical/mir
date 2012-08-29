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

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_FRAMEBUFFER_WINDOW_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_FRAMEBUFFER_WINDOW_H_

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace android
{

class AndroidFramebufferWindowQuery 
{
public:
    virtual ~AndroidFramebufferWindowQuery() {}

    virtual int android_visual_id() = 0;
    virtual EGLNativeWindowType android_native_window_type() = 0;
    virtual EGLConfig android_display_egl_config() = 0;
};


class AndroidFramebufferWindow : public AndroidFramebufferWindowQuery
{
public:
    AndroidFramebufferWindow() {}
    virtual ~AndroidFramebufferWindow() {}

    int android_visual_id() {return 9;}
    EGLNativeWindowType android_native_window_type() {return (EGLNativeWindowType) NULL;}
    EGLConfig android_display_egl_config() {return (EGLConfig)NULL;}
    

};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_FRAMEBUFFER_WINDOW_H_ */
