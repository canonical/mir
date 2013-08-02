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
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/egl_resources.h"
#include "android_display.h"
#include "android_display_buffer_factory.h"
#include "mir/geometry/rectangle.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

namespace
{

static EGLint const default_egl_context_attr[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

static EGLint const dummy_pbuffer_attribs[] =
{
    EGL_WIDTH, 1,
    EGL_HEIGHT, 1,
    EGL_NONE
};

class AndroidDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    AndroidDisplayConfiguration(geom::Size const& display_size)
        : configuration{mg::DisplayConfigurationOutputId{0},
                        mg::DisplayConfigurationCardId{0},
                        {mg::DisplayConfigurationMode{display_size,0.0f}},
                        geom::Size{0,0},
                        true,
                        true,
                        geom::Point{0,0},
                        0}
    {
    }
    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)>) const
    {
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
    {
        f(configuration);
    }

    void configure_output(mg::DisplayConfigurationOutputId, bool, geom::Point, size_t)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot configure output\n"));
    }

private:
    mg::DisplayConfigurationOutput configuration;
};

EGLDisplay create_and_initialize_display()
{
    EGLint major, minor;

    auto egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(std::runtime_error("eglGetDisplay failed\n"));

    if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("eglInitialize failure\n"));

    if ((major != 1) || (minor != 4))
        BOOST_THROW_EXCEPTION(std::runtime_error("must have EGL 1.4\n"));

    return egl_display;
}

class AndroidGLContext : public mg::GLContext
{
public:
    AndroidGLContext(EGLDisplay egl_display, EGLConfig egl_config, EGLContext egl_context_shared)
        : egl_display{egl_display},
          egl_context{egl_display,
                      eglCreateContext(egl_display, egl_config, egl_context_shared,
                                       default_egl_context_attr)},
          egl_surface{egl_display,
                      eglCreatePbufferSurface(egl_display, egl_config,
                                              dummy_pbuffer_attribs)}
    {
    }

    ~AndroidGLContext() noexcept
    {
        if (eglGetCurrentContext() == egl_context)
            release_current();
    }

    void make_current()
    {
        if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) == EGL_FALSE)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("could not activate dummy surface with eglMakeCurrent\n"));
        }
    }

    void release_current()
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

private:
    EGLDisplay const egl_display;
    mg::EGLContextStore const egl_context;
    mg::EGLSurfaceStore const egl_surface;
};

}

mga::AndroidDisplay::AndroidDisplay(const std::shared_ptr<AndroidFramebufferWindowQuery>& native_win,
                                    std::shared_ptr<AndroidDisplayBufferFactory> const& db_factory,
                                    std::shared_ptr<DisplayReport> const& display_report)
    : native_window{native_win},
      egl_display{create_and_initialize_display()},
      egl_config{native_window->android_display_egl_config(egl_display)},
      egl_context_shared{egl_display,
                         eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT,
                                          default_egl_context_attr)},
      egl_surface_dummy{egl_display,
                        eglCreatePbufferSurface(egl_display, egl_config,
                                                dummy_pbuffer_attribs)},
      display_buffer{db_factory->create_display_buffer(
                         native_window, egl_display, egl_context_shared)}
{
    display_report->report_successful_setup_of_native_resources();

    /* Make the shared context current */
    if (eglMakeCurrent(egl_display, egl_surface_dummy, egl_surface_dummy, egl_context_shared) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("could not activate dummy surface with eglMakeCurrent\n"));

    display_report->report_successful_egl_make_current_on_construction();
    display_report->report_successful_display_construction();
    display_report->report_egl_configuration(egl_display, egl_config);
}

mga::AndroidDisplay::~AndroidDisplay()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(egl_display);
}

void mga::AndroidDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    f(*display_buffer);
}

std::shared_ptr<mg::DisplayConfiguration> mga::AndroidDisplay::configuration()
{
    return std::make_shared<AndroidDisplayConfiguration>(display_buffer->view_area().size);
}

void mga::AndroidDisplay::configure(mg::DisplayConfiguration const&)
{
}

void mga::AndroidDisplay::register_configuration_change_handler(
    EventHandlerRegister&,
    DisplayConfigurationChangeHandler const&)
{
}

void mga::AndroidDisplay::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
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

auto mga::AndroidDisplay::the_cursor() -> std::weak_ptr<Cursor>
{
    return std::weak_ptr<Cursor>();
}

std::unique_ptr<mg::GLContext> mga::AndroidDisplay::create_gl_context()
{
    return std::unique_ptr<AndroidGLContext>{
        new AndroidGLContext{egl_display, egl_config, egl_context_shared}};
}
