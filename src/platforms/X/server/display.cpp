/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "display_configuration.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/egl_resources.h"
#include "display.h"
#include "display_buffer.h"
#include "gl_context.h"
#include "../debug.h"

#include <boost/throw_exception.hpp>
#include <fcntl.h>
#include <mutex>

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::Display::Display()
{
    CALLED

    dpy = XOpenDisplay(NULL);

    if (!dpy)
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot get a display"));

    GLint att[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };
    vi = glXChooseVisual(dpy, 0, att);

    int bits;
    glXGetConfig(dpy, vi, GLX_BUFFER_SIZE, &bits);
    std::cout<< "\tNumber of bits selected : " << bits << std::endl;

    int rgba;
    glXGetConfig(dpy, vi, GLX_RGBA, &rgba);
    std::cout<< "\tRGBA selected : " << (rgba ? "Yes" : "No") << std::endl;

    int alpha;
    glXGetConfig(dpy, vi, GLX_ALPHA_SIZE, &alpha);
    std::cout<< "\tALPHA exists: " << (alpha ? "Yes" : "No") << std::endl;

    if (bits == 32)
    {
        if (alpha)
            pf = mir_pixel_format_argb_8888;
        else
            pf = mir_pixel_format_xrgb_8888;
    }
    else if (bits == 24)
        pf = mir_pixel_format_bgr_888;

    auto root = DefaultRootWindow(dpy);

    if (!vi)
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot get a display"));

    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask;

    win = XCreateWindow(dpy, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(dpy, win);
    XStoreName(dpy, win, "Mir on X");

    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);

    glXMakeCurrent(dpy, win, glc);

    XEvent xev;
    while(1)
    {
       XNextEvent(dpy, &xev);

       if(xev.type == Expose)
       {
           XGetWindowAttributes(dpy, win, &gwa);
           glViewport(0, 0, gwa.width, gwa.height);

           glClearColor(1.0, 1.0, 1.0, 1.0);
           glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
           glXSwapBuffers(dpy, win);
           display_group = std::make_unique<mgx::DisplayGroup>(std::make_unique<mgx::DisplayBuffer>(geom::Size{gwa.width, gwa.height}));
           return;
       }
    }
}

mgx::Display::~Display() noexcept
{
    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, glc);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

void mgx::Display::for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f)
{
	CALLED
    f(*display_group);
#if 0
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    f(displays);
#endif
}

std::unique_ptr<mg::DisplayConfiguration> mgx::Display::configuration() const
{
	CALLED
#if 0
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};
    return std::unique_ptr<mg::DisplayConfiguration>(new mgx::DisplayConfiguration(config));
#else
    return std::make_unique<mgx::DisplayConfiguration>(pf, gwa.width, gwa.height);
#endif
}

void mgx::Display::configure(mg::DisplayConfiguration const& /*new_configuration*/)
{
	CALLED
#if 0
    if (!new_configuration.valid())
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid or inconsistent display configuration"));

    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    new_configuration.for_each_output([this](mg::DisplayConfigurationOutput const& output)
    {
        if (output.current_format != config[output.id].current_format)
            BOOST_THROW_EXCEPTION(std::logic_error("could not change display buffer format"));

        config[output.id].orientation = output.orientation;
        if (config.primary().id == output.id)
        {
            power_mode(mga::DisplayName::primary, *hwc_config, config.primary(), output.power_mode);
            displays.configure(mga::DisplayName::primary, output.power_mode, output.orientation);
        }
        else if (config.external().connected)
        {
            power_mode(mga::DisplayName::external, *hwc_config, config.external(), output.power_mode);
            displays.configure(mga::DisplayName::external, output.power_mode, output.orientation);
        }
    });
#endif
}

void mgx::Display::register_configuration_change_handler(
    EventHandlerRegister& /* event_handler */,
    DisplayConfigurationChangeHandler const& /* change_handler */)
{
	CALLED
#if 0
    event_handler.register_fd_handler({display_change_pipe->read_pipe}, this, [change_handler, this](int){
        change_handler();
        display_change_pipe->ack_change();
    });
#endif
}

void mgx::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
	CALLED
}

void mgx::Display::pause()
{
	CALLED
}

void mgx::Display::resume()
{
	CALLED
}

auto mgx::Display::create_hardware_cursor(std::shared_ptr<mg::CursorImage> const& /* initial_image */) -> std::shared_ptr<Cursor>
{
	CALLED
    return nullptr;
}

std::unique_ptr<mg::GLContext> mgx::Display::create_gl_context()
{
	CALLED
    return std::make_unique<mgx::XGLContext>(dpy, win, glc);
}
