/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "android_framebuffer_window_query.h"
#include <system/window.h>
#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{

class AndroidFramebufferWindow : public AndroidFramebufferWindowQuery
{
public:
    AndroidFramebufferWindow(const std::shared_ptr<ANativeWindow>& anw);
    virtual ~AndroidFramebufferWindow() {}

    EGLNativeWindowType android_native_window_type() const;
    EGLConfig android_display_egl_config(EGLDisplay) const;

private:
    const std::shared_ptr<ANativeWindow> native_window;

};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_FRAMEBUFFER_WINDOW_H_ */
