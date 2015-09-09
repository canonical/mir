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
#include "mir/graphics/display_report.h"
#include "mir/graphics/egl_resources.h"
#include "mir/graphics/egl_error.h"
#include "display_configuration.h"
#include "display.h"
#include "display_buffer.h"
#include "gl_context.h"

#include <boost/throw_exception.hpp>
#include <fcntl.h>
#include <mutex>

#define MIR_LOG_COMPONENT "x11-display"
#include "mir/log.h"

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::X11EGLDisplay::X11EGLDisplay(::Display* x_dpy)
    : egl_dpy{eglGetDisplay(x_dpy)}
{
    if (!egl_dpy)
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get an egl display"));

    EGLint egl_major, egl_minor;
    if (!eglInitialize(egl_dpy, &egl_major, &egl_minor))
        BOOST_THROW_EXCEPTION(mg::egl_error("eglInitialize failed"));

    mir::log_info("EGL Version %d.%d", egl_major, egl_minor);
}

mgx::X11EGLDisplay::~X11EGLDisplay()
{
    eglTerminate(egl_dpy);
}

mgx::X11EGLDisplay::operator EGLDisplay() const
{
    return egl_dpy;
}

mgx::X11Window::X11Window(::Display* x_dpy, EGLDisplay egl_dpy, int width, int height)
    : x_dpy{x_dpy}
{
    EGLint const att[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    auto root = XDefaultRootWindow(x_dpy);

    EGLint num_configs;
    if (!eglChooseConfig(egl_dpy, att, &config, 1, &num_configs))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get an EGL config"));

    mir::log_info("%d configs found", num_configs);

    if (num_configs <= 0)
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get an EGL config"));

    EGLint vid;
    if (!eglGetConfigAttrib(egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get config attrib"));

    XVisualInfo visTemplate;
    int num_visuals;
    visTemplate.visualid = vid;
    auto visInfo = XGetVisualInfo(x_dpy, VisualIDMask, &visTemplate, &num_visuals);
    if (!visInfo)
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get visual info"));

    mir::log_info("%d visuals found", num_visuals);

    XSetWindowAttributes attr;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(x_dpy, root, visInfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask |
                      ExposureMask        |
                      KeyPressMask        |
                      KeyReleaseMask      |
                      ButtonPressMask     |
                      ButtonReleaseMask   |
                      FocusChangeMask     |
                      PointerMotionMask;

    depth = visInfo->depth;

    mir::log_info("Pixel depth = %d", depth);

    auto mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    win = XCreateWindow(x_dpy, root, 0, 0, width, height,
                        0, depth, InputOutput,
                        visInfo->visual, mask, &attr);

    XFree(visInfo);

    {
        char const * const title = "Mir On X";
        XSizeHints sizehints;

        // TODO: Due to a bug, resize doesn't work after XGrabKeyboard under Unity.
        //       For now, make window unresizeable.
        //     http://stackoverflow.com/questions/14555703/x11-unable-to-move-window-after-xgrabkeyboard
        sizehints.base_width = width;
        sizehints.base_height = height;
        sizehints.min_width = width;
        sizehints.min_height = height;
        sizehints.max_width = width;
        sizehints.max_height = height;
        sizehints.flags = PSize | PMinSize | PMaxSize;

        XSetNormalHints(x_dpy, win, &sizehints);
        XSetStandardProperties(x_dpy, win, title, title, None, (char **)NULL, 0, &sizehints);

        XWMHints wm_hints = {
            (InputHint|StateHint), // fields in this structure that are defined
            True,                  // does this application rely on the window manager
                                   // to get keyboard input? Yes, if this is False,
                                   // XGrabKeyboard doesn't work reliably.
            NormalState,           // initial_state
            0,                     // icon_pixmap
            0,                     // icon_window
            0, 0,                  // initial position of icon
            0,                     // pixmap to be used as mask for icon_pixmap
            0                      // id of related window_group
        };

        XSetWMHints(x_dpy, win, &wm_hints);
    }

    XMapWindow(x_dpy, win);

    XEvent xev;
    do 
    {
        XNextEvent(x_dpy, &xev);
    }
    while (xev.type != Expose);
}

mgx::X11Window::~X11Window()
{
    XDestroyWindow(x_dpy, win);
}

mgx::X11Window::operator Window() const
{
    return win;
}

EGLConfig mgx::X11Window::egl_config() const
{
    return config;
}

unsigned int mgx::X11Window::color_depth() const
{
    return depth;
}

mgx::X11EGLContext::X11EGLContext(EGLDisplay egl_dpy, EGLConfig config)
    : egl_dpy{egl_dpy}
{
    static const EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE };

    egl_ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs);
    if (!egl_ctx)
        BOOST_THROW_EXCEPTION(mg::egl_error("eglCreateContext failed"));
}

mgx::X11EGLContext::~X11EGLContext()
{
    eglDestroyContext(egl_dpy, egl_ctx);
}

mgx::X11EGLContext::operator EGLContext() const
{
    return egl_ctx;
}

mgx::X11EGLSurface::X11EGLSurface(EGLDisplay egl_dpy, EGLConfig config, Window win)
    : egl_dpy{egl_dpy}, egl_surf{eglCreateWindowSurface(egl_dpy, config, win, NULL)}
{
    if (!egl_surf)
        BOOST_THROW_EXCEPTION(mg::egl_error("eglCreateWindowSurface failed"));
}

mgx::X11EGLSurface::~X11EGLSurface()
{
    eglDestroySurface(egl_dpy, egl_surf);
}

mgx::X11EGLSurface::operator EGLSurface() const
{
    return egl_surf;
}

mgx::Display::Display(::Display* x_dpy)
    : egl_display{X11EGLDisplay(x_dpy)},
      display_width{1280},
      display_height{1024},
      win{X11Window(x_dpy,
                    egl_display,
                    display_width,
                    display_height)},
      egl_context{X11EGLContext(egl_display,
                                win.egl_config())},
      egl_surface{X11EGLSurface(egl_display,
                                win.egl_config(),
                                win)},
                                orientation{mir_orientation_normal}
{
    if (win.color_depth() == 24)
        pf = mir_pixel_format_bgr_888;
    else
        BOOST_THROW_EXCEPTION(std::runtime_error("Unsupported pixel format"));

    display_group = std::make_unique<mgx::DisplayGroup>(
        std::make_unique<mgx::DisplayBuffer>(geom::Size{display_width, display_height},
                                             egl_display,
                                             egl_surface,
                                             egl_context,
                                             orientation));
}

mgx::Display::~Display() noexcept
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void mgx::Display::for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f)
{
    f(*display_group);
}

std::unique_ptr<mg::DisplayConfiguration> mgx::Display::configuration() const
{
    return std::make_unique<mgx::DisplayConfiguration>(pf, display_width, display_height, orientation);
}

void mgx::Display::configure(mg::DisplayConfiguration const& new_configuration)
{
    MirOrientation o = mir_orientation_normal;

    new_configuration.for_each_output([&](DisplayConfigurationOutput const& conf_output)
    {
        o = conf_output.orientation;
    });

    orientation = o;
    display_group->set_orientation(orientation);
}

void mgx::Display::register_configuration_change_handler(
    EventHandlerRegister& /* event_handler*/,
    DisplayConfigurationChangeHandler const& /*change_handler*/)
{
}

void mgx::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mgx::Display::pause()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::pause()' not yet supported on x11 platform"));
}

void mgx::Display::resume()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::resume()' not yet supported on x11 platform"));
}

auto mgx::Display::create_hardware_cursor(std::shared_ptr<mg::CursorImage> const& /* initial_image */) -> std::shared_ptr<Cursor>
{
    return nullptr;
}

std::unique_ptr<mg::GLContext> mgx::Display::create_gl_context()
{
    return std::make_unique<mgx::XGLContext>(egl_display, egl_surface, egl_context);
}
