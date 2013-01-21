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

#include "android_framebuffer_window.h"

#include <boost/exception/all.hpp>

#include <stdexcept>

namespace mga = mir::graphics::android;

namespace
{
static const EGLint default_egl_config_attr [] =
{
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};
}
mga::AndroidFramebufferWindow::AndroidFramebufferWindow(const std::shared_ptr<ANativeWindow>& anw)
    :
    native_window(anw)
{
}

EGLNativeWindowType mga::AndroidFramebufferWindow::android_native_window_type() const
{
    return (EGLNativeWindowType) native_window.get();
}

EGLConfig mga::AndroidFramebufferWindow::android_display_egl_config(EGLDisplay egl_display) const
{
    EGLint num_match_configs;
    int num_potential_configs;
    EGLConfig* config_slots;
    EGLConfig egl_config;
    eglGetConfigs(egl_display, NULL, 0, &num_potential_configs);
    config_slots = new EGLConfig[num_potential_configs];

    /* upon return, this will fill config_slots[0:num_match_configs] with the matching */
    eglChooseConfig(egl_display, default_egl_config_attr, config_slots, num_potential_configs, &num_match_configs );

    int android_native_id;
    native_window->query(native_window.get(), NATIVE_WINDOW_FORMAT, &android_native_id);

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
            break;
        }
    }
    delete config_slots;

    if (!found)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not select EGL config"));

    return egl_config;
}
