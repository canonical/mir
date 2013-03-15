/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/display_configuration.h"
#include "android_display.h"
#include "mir/geometry/rectangle.h"

#include <boost/throw_exception.hpp>

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

class NullDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)>)
    {
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)>)
    {
    }
};
}

mga::AndroidDisplay::AndroidDisplay(const std::shared_ptr<AndroidFramebufferWindowQuery>& native_win)
    : native_window(native_win)
{
    EGLint major, minor;

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(std::runtime_error("eglGetDisplay failed\n"));

    if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("eglInitialize failure\n"));

    if ((major != 1) || (minor != 4))
        BOOST_THROW_EXCEPTION(std::runtime_error("must have EGL 1.4\n"));

    egl_config = native_window->android_display_egl_config(egl_display);
    EGLNativeWindowType native_win_type = native_window->android_native_window_type();
    egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_win_type, NULL);
    if(egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not create egl surface\n"));

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, default_egl_context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not create egl context\n"));

    make_current();
}

mga::AndroidDisplay::~AndroidDisplay()
{
    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);
}

geom::Rectangle mga::AndroidDisplay::view_area() const
{
    int display_width, display_height;
    eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &display_width);
    eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &display_height);
    geom::Width w(display_width);
    geom::Height h(display_height);

    geom::Point pt { geom::X{0},
                     geom::Y{0}};
    geom::Size sz{w,h};
    geom::Rectangle rect{pt, sz};
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

void mga::AndroidDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    f(*this);
}

std::shared_ptr<mg::DisplayConfiguration> mga::AndroidDisplay::configuration()
{
    return std::make_shared<NullDisplayConfiguration>();
}

void mga::AndroidDisplay::make_current()
{
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not activate surface with eglMakeCurrent\n"));
}
