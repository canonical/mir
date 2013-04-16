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

#include "android_framebuffer_window.h"

#include <boost/throw_exception.hpp>

#include <vector>
#include <algorithm>
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
    int num_potential_configs, android_native_id;
    EGLint num_match_configs;

    eglGetConfigs(egl_display, NULL, 0, &num_potential_configs);
    std::vector<EGLConfig> config_slots(num_potential_configs);

    /* upon return, this will fill config_slots[0:num_match_configs] with the matching */
    eglChooseConfig(egl_display, default_egl_config_attr,
                    config_slots.data(), num_potential_configs, &num_match_configs);
    config_slots.resize(num_match_configs);

    /* why check manually for EGL_NATIVE_VISUAL_ID instead of using eglChooseConfig? the egl
     * specification does not list EGL_NATIVE_VISUAL_ID as something it will check for in
     * eglChooseConfig */
    native_window->query(native_window.get(), NATIVE_WINDOW_FORMAT, &android_native_id);
    auto const pegl_config = std::find_if(begin(config_slots), end(config_slots),
        [&](EGLConfig& current) -> bool
        {
            int visual_id;
            eglGetConfigAttrib(egl_display, current, EGL_NATIVE_VISUAL_ID, &visual_id);
            return (visual_id == android_native_id);
        });

    if (pegl_config == end(config_slots))
        BOOST_THROW_EXCEPTION(std::runtime_error("could not select EGL config for use with framebuffer"));

    return *pegl_config;
}
