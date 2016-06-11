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
#include "mir/graphics/virtual_output.h"
#include "mir/graphics/gl_config.h"
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

mgx::X11Window::X11Window(::Display* x_dpy,
                          EGLDisplay egl_dpy,
                          geom::Size const size,
                          GLConfig const& gl_config)
    : x_dpy{x_dpy}
{
    EGLint const att[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, gl_config.depth_buffer_bits(),
        EGL_STENCIL_SIZE, gl_config.stencil_buffer_bits(),
        EGL_RENDERABLE_TYPE, MIR_SERVER_EGL_OPENGL_BIT,
        EGL_NONE
    };

    auto root = XDefaultRootWindow(x_dpy);

    EGLint num_configs;
    if (!eglChooseConfig(egl_dpy, att, &config, 1, &num_configs))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get an EGL config"));

    mir::log_info("%d config(s) found", num_configs);

    if (num_configs <= 0)
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get an EGL config"));

    EGLint vid;
    if (!eglGetConfigAttrib(egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get config attrib"));

    XVisualInfo visTemplate;
    std::memset(&visTemplate, 0, sizeof visTemplate);
    int num_visuals = 0;
    visTemplate.visualid = vid;
    auto visInfo = XGetVisualInfo(x_dpy, VisualIDMask, &visTemplate, &num_visuals);
    if (!visInfo || !num_visuals)
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get visual info, or no matching visuals"));

    mir::log_info("%d visual(s) found", num_visuals);
    mir::log_info("Using the first one :");
    mir::log_info("ID\t\t:\t%d", visInfo->visualid);
    mir::log_info("screen\t:\t%d", visInfo->screen);
    mir::log_info("depth\t\t:\t%d", visInfo->depth);
    mir::log_info("red_mask\t:\t0x%0X", visInfo->red_mask);
    mir::log_info("green_mask\t:\t0x%0X", visInfo->green_mask);
    mir::log_info("blue_mask\t:\t0x%0X", visInfo->blue_mask);
    mir::log_info("colormap_size\t:\t%d", visInfo->colormap_size);
    mir::log_info("bits_per_rgb\t:\t%d", visInfo->bits_per_rgb);

    r_mask = visInfo->red_mask;

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
                      EnterWindowMask     |
                      LeaveWindowMask     |
                      PointerMotionMask;

    auto mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    win = XCreateWindow(x_dpy, root, 0, 0,
                        size.width.as_int(), size.height.as_int(),
                        0, visInfo->depth, InputOutput,
                        visInfo->visual, mask, &attr);

    XFree(visInfo);

    {
        char const * const title = "Mir On X";
        XSizeHints sizehints;

        // TODO: Due to a bug, resize doesn't work after XGrabKeyboard under Unity.
        //       For now, make window unresizeable.
        //     http://stackoverflow.com/questions/14555703/x11-unable-to-move-window-after-xgrabkeyboard
        sizehints.base_width = size.width.as_int();
        sizehints.base_height = size.height.as_int();
        sizehints.min_width = size.width.as_int();
        sizehints.min_height = size.height.as_int();
        sizehints.max_width = size.width.as_int();
        sizehints.max_height = size.height.as_int();
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

unsigned long mgx::X11Window::red_mask() const
{
    return r_mask;
}

mgx::X11EGLContext::X11EGLContext(EGLDisplay egl_dpy, EGLConfig config)
    : egl_dpy{egl_dpy}
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    static const EGLint ctx_attribs[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
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

mgx::Display::Display(::Display* x_dpy,
                      geom::Size const size,
                      GLConfig const& gl_config)
    : egl_display{X11EGLDisplay(x_dpy)},
      size{size},
      win{X11Window(x_dpy,
                    egl_display,
                    size,
                    gl_config)},
      egl_context{X11EGLContext(egl_display,
                                win.egl_config())},
      egl_surface{X11EGLSurface(egl_display,
                                win.egl_config(),
                                win)},
                                orientation{mir_orientation_normal}
{
    auto red_mask = win.red_mask();
    pf = (red_mask == 0xFF0000 ? mir_pixel_format_argb_8888
                               : mir_pixel_format_abgr_8888);

    display_group = std::make_unique<mgx::DisplayGroup>(
        std::make_unique<mgx::DisplayBuffer>(size,
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
    return std::make_unique<mgx::DisplayConfiguration>(pf, size, orientation);
}

void mgx::Display::configure(mg::DisplayConfiguration const& new_configuration)
{
    if (!new_configuration.valid())
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("Invalid or inconsistent display configuration"));
    }

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

std::unique_ptr<mg::VirtualOutput> mgx::Display::create_virtual_output(int /*width*/, int /*height*/)
{
    return nullptr;
}
