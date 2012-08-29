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
#include "mir/geometry/rectangle.h"

#include <stdexcept>
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

static const EGLint attr [] =
{
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

mga::AndroidDisplay::AndroidDisplay(const std::shared_ptr<AndroidFramebufferWindow>& native_win)
 : native_window(native_win)
{
    EGLint major, minor;

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
    {
        throw std::runtime_error("eglGetDisplay failed\n");
    }
    
    if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
    {
        throw std::runtime_error("eglInitialize failure\n");
    }
    /* todo: we could adapt to different versions. */
    if ((major != 1) || (minor != 4))
    {
        throw std::runtime_error("must have EGL 1.4\n");
    }
   
    EGLConfig egl_config;
    EGLint num_match_configs;
    int num_potential_configs;
    EGLint* config_slots;
    eglGetConfigs(egl_display, NULL, 0, &num_potential_configs);
    config_slots = new EGLint[num_potential_configs]; 

    eglChooseConfig(egl_display, attr, &egl_config, num_potential_configs, &num_match_configs );

    delete config_slots;
/*    int android_native_id =*/ native_window->getAndroidVisualId();
     

}
    
geom::Rectangle mga::AndroidDisplay::view_area()
{
    geom::Rectangle rect;
    return rect;
}

void mga::AndroidDisplay::notify_update()
{

}
