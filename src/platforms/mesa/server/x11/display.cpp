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

mgx::X11EGLDisplay::X11EGLDisplay(::Display *x_dpy)
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

mgx::X11Window::X11Window(::Display *x_dpy, EGLDisplay egl_dpy, int width, int height)
    : x_dpy{x_dpy}
{
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

    auto root = XDefaultRootWindow(x_dpy);

    EGLint num_configs;
    if (!eglChooseConfig(egl_dpy, att, &config, 1, &num_configs))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get an EGL config"));

    assert(config);
    assert(num_configs > 0);

    EGLint vid;
    if (!eglGetConfigAttrib(egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get config attrib"));

    XVisualInfo visTemplate;
    int num_visuals;
    visTemplate.visualid = vid;
    auto visInfo = XGetVisualInfo(x_dpy, VisualIDMask, &visTemplate, &num_visuals);
    if (!visInfo)
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get visual info"));

    XSetWindowAttributes attr;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(x_dpy, root, visInfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask |
                      ExposureMask        |
                      KeyPressMask        |
                      KeyReleaseMask      |
                      ButtonPressMask     |
                      ButtonReleaseMask;

    auto mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    win = XCreateWindow(x_dpy, root, 0, 0, width, height,
                        0, visInfo->depth, InputOutput,
                        visInfo->visual, mask, &attr);

    mir::log_info("Pixel depth = %d", visInfo->depth);

    //TODO: handle others
    assert(visInfo->depth==24);

    XFree(visInfo);

    XMapWindow(x_dpy, win);
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

mgx::Display::Display(::Display *dpy)
    : x_dpy{dpy},
      egl_display{X11EGLDisplay(dpy)},
      display_width{1280},
      display_height{1024},
      win{X11Window(dpy,
                    egl_display,
                    display_width,
                    display_height)},
      egl_context{X11EGLContext(egl_display,
                                win.egl_config())},
      egl_surface{X11EGLSurface(egl_display,
                                win.egl_config(),
                                win)}
{
    // TODO: read from the chosen config
    pf = mir_pixel_format_bgr_888;

    // Make window nonresizeable
    // TODO: Make sizing possible
    {
        char const * const title = "Mir On X";
        XSizeHints sizehints;

        sizehints.x = 0;
        sizehints.y = 0;
        sizehints.base_width = display_width;
        sizehints.base_height = display_height;
        sizehints.min_width  = display_width;
        sizehints.min_height = display_height;
        sizehints.max_width = display_width;
        sizehints.max_height = display_height;
        sizehints.flags = USSize | USPosition | PMinSize | PMaxSize;

        XSetNormalHints(x_dpy, win, &sizehints);
        XSetStandardProperties(x_dpy, win, title, title, None, (char **)NULL, 0, &sizehints);
    }

    display_group = std::make_unique<mgx::DisplayGroup>(
        std::make_unique<mgx::DisplayBuffer>(geom::Size{display_width, display_height},
                                             egl_display,
                                             egl_surface,
                                             egl_context));
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
    return std::make_unique<mgx::DisplayConfiguration>(pf, display_width, display_height);
}

void mgx::Display::configure(mg::DisplayConfiguration const& /*new_configuration*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::configure()' not yet supported on x11 platform"));
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
