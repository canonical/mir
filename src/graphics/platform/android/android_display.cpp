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

#include "system/window.h"

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
   
    EGLint num_match_configs;
    int num_potential_configs;
    EGLConfig* config_slots;
    eglGetConfigs(egl_display, NULL, 0, &num_potential_configs);
    config_slots = new EGLConfig[num_potential_configs]; 

    /* upon return, this will fill config_slots[0:num_match_configs] with the matching */
    eglChooseConfig(egl_display, attr, config_slots, num_potential_configs, &num_match_configs );
    int android_native_id = native_window->getAndroidVisualId();

    /* why check manually for EGL_NATIVE_VISUAL_ID instead of using eglChooseConfig? the egl
     * specification does not list EGL_NATIVE_VISUAL_ID as something it will check for in
     * eglChooseConfig */
    bool found=false; 
    for(int i=0; i < num_match_configs; i++)
    {
        int visual_id;
        eglGetConfigAttrib(egl_display, config_slots[i], EGL_NATIVE_VISUAL_ID, &visual_id);
        if(visual_id == android_native_id)
        {
            egl_config = config_slots[i];
            found = true;
        } 
    }
    delete config_slots;

    if (!found)
        throw std::runtime_error("could not select EGL config");

    EGLNativeWindowType native_win_type = native_window->getAndroidNativeEGLWindow();
    eglCreateWindowSurface(egl_display, egl_config, native_win_type, NULL);

}
    
geom::Rectangle mga::AndroidDisplay::view_area()
{
    geom::Rectangle rect;
    return rect;
}

void mga::AndroidDisplay::notify_update()
{

}
