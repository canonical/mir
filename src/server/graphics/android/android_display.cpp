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

#include "mir/graphics/platform.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_report.h"
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

mga::AndroidDisplay::AndroidDisplay(const std::shared_ptr<AndroidFramebufferWindowQuery>& native_win,
                                    std::shared_ptr<DisplayReport> const& display_report)
    : egl_display{EGL_NO_DISPLAY},
      egl_surface{EGL_NO_SURFACE},
      native_window{native_win},
      egl_config{0},
      egl_context{EGL_NO_CONTEXT},
      egl_context_shared{EGL_NO_CONTEXT},
      egl_surface_dummy{EGL_NO_SURFACE}
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

    display_report->report_successful_setup_of_native_resources();

    egl_context_shared = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, default_egl_context_attr);
    if (egl_context_shared == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not create egl context dummy\n"));

    egl_context = eglCreateContext(egl_display, egl_config, egl_context_shared, default_egl_context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not create egl context\n"));

    EGLint pbuffer_attribs[]{EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    egl_surface_dummy = eglCreatePbufferSurface(egl_display, egl_config, pbuffer_attribs);
    if (egl_surface_dummy == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not create dummy egl surface\n"));

    if (eglMakeCurrent(egl_display, egl_surface_dummy, egl_surface_dummy, egl_context_shared) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not activate dummy surface with eglMakeCurrent\n"));

    display_report->report_successful_egl_make_current_on_construction();
    display_report->report_successful_display_construction();
    display_report->report_egl_configuration(egl_display, egl_config);
}

mga::AndroidDisplay::~AndroidDisplay()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglDestroyContext(egl_display, egl_context_shared);
    eglDestroySurface(egl_display, egl_surface_dummy);
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

void mga::AndroidDisplay::post_update()
{
    if (eglSwapBuffers(egl_display, egl_surface) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("eglSwapBuffers failure\n"));
    }
}

void mga::AndroidDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    f(*this);
}

std::shared_ptr<mg::DisplayConfiguration> mga::AndroidDisplay::configuration()
{
    return std::make_shared<NullDisplayConfiguration>();
}

void mga::AndroidDisplay::register_pause_resume_handlers(
    MainLoop& /*main_loop*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mga::AndroidDisplay::pause()
{
}

void mga::AndroidDisplay::resume()
{
}

void mga::AndroidDisplay::make_current()
{
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("could not activate surface with eglMakeCurrent\n"));
    }
}

void mga::AndroidDisplay::release_current()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}


auto mga::AndroidDisplay::the_cursor() -> std::weak_ptr<Cursor>
{
    return std::weak_ptr<Cursor>();
}
