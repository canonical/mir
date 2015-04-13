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
    : display_width{1280}, display_height{1024}
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

    x_dpy = XOpenDisplay(NULL);
    if (!x_dpy)
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot get a display"));

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
        EGL_ALPHA_SIZE, 1,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint ctx_attribs[] = {
       EGL_CONTEXT_CLIENT_VERSION, 2,
       EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_ES_API);

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
                        visInfo->visual, mask, &attr );

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

//    if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx))
//        BOOST_THROW_EXCEPTION(std::logic_error("eglMakeCurrent failed"));

//    glViewport(0, 0, (GLint) width, (GLint) height);

#if 0
    GLint att[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };
    vi = glXChooseVisual(dpy, 0, att);

    int maj, min;
    glXQueryVersion(dpy, &maj, &min);
    std::cout<< "\tglX version : (" << maj << ',' << min << ')' << std::endl;

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
    swa.event_mask = ExposureMask;

    win = XCreateWindow(dpy, root, 0, 0, 1280, 1024, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(dpy, win);
    XStoreName(dpy, win, "Mir on X");

    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);

    glXMakeCurrent(dpy, win, glc);

    XEvent xev;
    while(1)
    {
       XNextEvent(dpy, &xev);

       if(xev.type == Expose)
           break;
    }
    XGetWindowAttributes(dpy, win, &gwa);
    glViewport(0, 0, gwa.width, gwa.height);

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glXSwapBuffers(dpy, win);
#endif

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
    XCloseDisplay(x_dpy);
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
