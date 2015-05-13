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
#include "debug.h"

#include <boost/throw_exception.hpp>
#include <fcntl.h>
#include <mutex>

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::Display::Display(::Display *dpy)
    : x_dpy{dpy}, display_width{1280}, display_height{1024}
{
    EGLint egl_major, egl_minor;
    EGLConfig config;
    EGLint num_configs;
    EGLint vid;
    Window root;
    XVisualInfo *visInfo, visTemplate;
    XSetWindowAttributes attr;
    int num_visuals;
    int scrno;
    unsigned long mask;
    char const * const title = "Mir On X";

    CALLED

    egl_dpy = eglGetDisplay(x_dpy);
    if (!egl_dpy)
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot get an egl display"));

    if (!eglInitialize(egl_dpy, &egl_major, &egl_minor))
        BOOST_THROW_EXCEPTION(std::logic_error("eglInitialize failed"));

    std::cout<<"EGL Version "<<egl_major<<'.'<<egl_minor << std::endl;

    EGLint const att[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 5,
        EGL_GREEN_SIZE, 5,
        EGL_BLUE_SIZE, 5,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint ctx_attribs[] = {
       EGL_CONTEXT_CLIENT_VERSION, 2,
       EGL_NONE
    };

    scrno = DefaultScreen(x_dpy);
    root = RootWindow(x_dpy, scrno);

    if (!eglChooseConfig(egl_dpy, att, &config, 1, &num_configs))
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot get an EGL config"));

    assert(config);
    assert(num_configs > 0);

    if (!eglGetConfigAttrib(egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid))
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot get config attrib"));

    visTemplate.visualid = vid;
    visInfo = XGetVisualInfo(x_dpy, VisualIDMask, &visTemplate, &num_visuals);
    if (!visInfo)
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot get visual info"));

    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(x_dpy, root, visInfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    win = XCreateWindow(x_dpy, root, 0, 0, 1280, 1024,
                        0, visInfo->depth, InputOutput,
                        visInfo->visual, mask, &attr);

    std::cout<<"depth="<<visInfo->depth<<std::endl;

    //TODO: handle others
    assert(visInfo->depth==24);

    pf = mir_pixel_format_bgr_888;

    {
        XSizeHints sizehints;
        sizehints.x = 0;
        sizehints.y = 0;
        sizehints.width  = display_width;
        sizehints.height = display_height;
        sizehints.flags = USSize | USPosition;
        XSetNormalHints(x_dpy, win, &sizehints);
        XSetStandardProperties(x_dpy, win, title, title,
                                   None, (char **)NULL, 0, &sizehints);
    }

    egl_ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs);
    if (!egl_ctx)
        BOOST_THROW_EXCEPTION(std::logic_error("eglCreateContext failed"));

    egl_surf = eglCreateWindowSurface(egl_dpy, config, win, NULL);
    if (!egl_surf)
        BOOST_THROW_EXCEPTION(std::logic_error("eglCreateWindowSurface failed"));

    /* sanity checks */
    {
       EGLint val;
       eglQueryContext(egl_dpy, egl_ctx, EGL_CONTEXT_CLIENT_VERSION, &val);
       assert(val == 2);
       eglQuerySurface(egl_dpy, egl_surf, EGL_WIDTH, &val);
       assert(val == display_width);
       eglQuerySurface(egl_dpy, egl_surf, EGL_HEIGHT, &val);
       assert(val == display_height);
       assert(eglGetConfigAttrib(egl_dpy, config, EGL_SURFACE_TYPE, &val));
       assert(val & EGL_WINDOW_BIT);
    }

    XFree(visInfo);

    XMapWindow(x_dpy, win);

    display_group = std::make_unique<mgx::DisplayGroup>(
        std::make_unique<mgx::DisplayBuffer>(geom::Size{display_width, display_height},
                                             egl_dpy,
                                             egl_surf,
                                             egl_ctx));
}

mgx::Display::~Display() noexcept
{
    eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(egl_dpy, egl_ctx);
    eglDestroySurface(egl_dpy, egl_surf);
    eglTerminate(egl_dpy);

    XDestroyWindow(x_dpy, win);
//    XCloseDisplay(x_dpy);
}

void mgx::Display::for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f)
{
	CALLED
    f(*display_group);
}

std::unique_ptr<mg::DisplayConfiguration> mgx::Display::configuration() const
{
	CALLED
//    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};
    return std::make_unique<mgx::DisplayConfiguration>(pf, display_width, display_height);
}

void mgx::Display::configure(mg::DisplayConfiguration const& /*new_configuration*/)
{
	CALLED
}

void mgx::Display::register_configuration_change_handler(
    EventHandlerRegister& /* event_handler*/,
    DisplayConfigurationChangeHandler const& /*change_handler*/)
{
	CALLED
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
    return std::make_unique<mgx::XGLContext>(egl_dpy, egl_surf, egl_ctx);
}
