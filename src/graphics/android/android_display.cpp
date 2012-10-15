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

#include "mir/graphics/platform.h"
#include "mir/graphics/android/android_display.h"
#include "mir/geometry/rectangle.h"

#include <stdexcept>

#include <GLES2/gl2.h>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

namespace
{
static const EGLint default_egl_context_attr [] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};
}

mga::AndroidDisplay::AndroidDisplay(const std::shared_ptr<AndroidFramebufferWindowQuery>& native_win)
    : native_window(native_win)
{
    EGLint major, minor;

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
        throw std::runtime_error("eglGetDisplay failed\n");

    if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
        throw std::runtime_error("eglInitialize failure\n");

    if ((major != 1) || (minor != 4))
        throw std::runtime_error("must have EGL 1.4\n");

    egl_config = native_window->android_display_egl_config(egl_display);
    EGLNativeWindowType native_win_type = native_window->android_native_window_type();
    egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_win_type, NULL);
    if(egl_surface == EGL_NO_SURFACE)
        throw std::runtime_error("could not create egl surface\n");

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, default_egl_context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        throw std::runtime_error("could not create egl context\n");

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) == EGL_FALSE)
        throw std::runtime_error("could not activate surface with eglMakeCurrent\n");
}

mga::AndroidDisplay::~AndroidDisplay()
{
    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);
}

/* todo: kdub: this will return some sort of information concerning the coordinate space
               of the displays. once we have a not-stubbed rect class and a coordinate
               space agreed upon, we can implement this */
geom::Rectangle mga::AndroidDisplay::view_area() const
{
    geom::Rectangle rect;
    return rect;
}

void mga::AndroidDisplay::clear()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

bool mga::AndroidDisplay::post_update()
{
    if (eglSwapBuffers(egl_display, egl_surface) == EGL_FALSE)
        return false;
    return true;
}
