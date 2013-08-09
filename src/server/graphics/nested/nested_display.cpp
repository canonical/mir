/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include "nested_display.h"
#include "nested_display_configuration.h"
#include "nested_gl_context.h"

#include <cstring>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

mgn::detail::MirSurfaceHandle::MirSurfaceHandle(MirConnection* connection)
{
    MirDisplayInfo egl_display_info;

    mir_connection_get_display_info(connection, &egl_display_info);
    if (!egl_display_info.supported_pixel_format_items)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to get the supported pixel format items."));

    MirSurfaceParameters const request_params =
        {
            "Mir nested display",
            int(egl_display_info.width),
            int(egl_display_info.height),
            egl_display_info.supported_pixel_format[0],
            mir_buffer_usage_hardware
        };

    mir_surface = mir_connection_create_surface_sync(connection, &request_params);

    if (!mir_surface_is_valid(mir_surface))
        BOOST_THROW_EXCEPTION(std::runtime_error(mir_surface_get_error_message(mir_surface)));
}

mgn::detail::MirSurfaceHandle::~MirSurfaceHandle() noexcept
{
    mir_surface_release_sync(mir_surface);
}

mgn::detail::EGLDisplayHandle::EGLDisplayHandle(MirConnection* connection)
{
    EGLNativeDisplayType const native_display = (EGLNativeDisplayType) mir_connection_get_egl_native_display(connection);
    if (!native_display)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to fetch EGL native display."));

    egl_display = eglGetDisplay(native_display);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to fetch EGL display."));
}

mgn::detail::EGLDisplayHandle::~EGLDisplayHandle() noexcept
{
    eglTerminate(egl_display);
}

#include <iostream> // DEBUG
mgn::NestedDisplay::NestedDisplay(MirConnection* connection, std::shared_ptr<mg::DisplayReport> const& display_report) :
    display_report{display_report},
    mir_surface{connection},
    egl_display{connection},
    egl_context{EGL_NO_CONTEXT}
{
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    int res, major, minor, n;

    res = eglInitialize(egl_display, &major, &minor);
    if ((res != EGL_TRUE) || (major != 1) || (minor != 4))
    {
        std::cerr << "DEBUG: egl_display=" << egl_display << std::endl;
        std::cerr << "DEBUG: res. . . . =" << res         << std::endl;
        std::cerr << "DEBUG: major. . . =" << major       << std::endl;
        std::cerr << "DEBUG: minor. . . =" << minor       << std::endl;
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to initialize EGL."));
    }

    res = eglChooseConfig(egl_display, attribs, &egl_config, 1, &n);
    if ((res != EGL_TRUE) || (n != 1))
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to choose EGL configuration."));

    EGLNativeWindowType native_window;
    if (!(native_window = (EGLNativeWindowType) mir_surface_get_egl_native_window(mir_surface)))
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to fetch EGL native window."));

    egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, NULL);
    if (egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to create EGL surface."));

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to create context."));

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to update EGL surface.\n"));

    egl_surface = eglGetCurrentSurface(EGL_DRAW);
    if (egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to get current EGL surface."));
}

mgn::NestedDisplay::~NestedDisplay()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglDestroyContext(egl_display, egl_context);
}

geom::Rectangle mgn::NestedDisplay::view_area() const
{
    int display_width, display_height;
    eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &display_width);
    eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &display_height);
    geom::Width w(display_width);
    geom::Height h(display_height);

    geom::Point pt {
        geom::X{0},
        geom::Y{0}
    };

    geom::Size sz{w,h};
    geom::Rectangle rect{pt, sz};
    return rect;
}

void mgn::NestedDisplay::post_update()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
    //mir_surface_swap_buffers_sync(mir_surface);
    //eglSwapBuffers(egl_display, egl_surface);
}

void mgn::NestedDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& /*f*/)
{
    // TODO
}

std::shared_ptr<mg::DisplayConfiguration> mgn::NestedDisplay::configuration()
{
    return std::make_shared<NullDisplayConfiguration>();
}

void mgn::NestedDisplay::configure(mg::DisplayConfiguration const& /*configuration*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

void mgn::NestedDisplay::register_configuration_change_handler(
        EventHandlerRegister& /*handlers*/,
        DisplayConfigurationChangeHandler const& /*conf_change_handler*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

void mgn::NestedDisplay::register_pause_resume_handlers(
        EventHandlerRegister& /*handlers*/,
        DisplayPauseHandler const& /*pause_handler*/,
        DisplayResumeHandler const& /*resume_handler*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

void mgn::NestedDisplay::pause()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

void mgn::NestedDisplay::resume()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

void mgn::NestedDisplay::make_current()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to MakeCurrent."));
}

void mgn::NestedDisplay::release_current()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

auto mgn::NestedDisplay::the_cursor()->std::weak_ptr<Cursor>
{
    return std::weak_ptr<Cursor>();
}

std::unique_ptr<mg::GLContext> mgn::NestedDisplay::create_gl_context()
{
    return std::unique_ptr<NestedGLContext>{
        new NestedGLContext{egl_display, egl_config, egl_context}};
}
