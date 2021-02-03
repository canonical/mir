/*
 * Copyright © 2015-2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miregl.h"
#include "wayland_app.h"

#include <cstring>

#include <GLES2/gl2.h>

#include <wayland-egl.h>

#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <set>

class MirEglApp : public WaylandApp
{
public:
    MirEglApp(wl_display* display);

    void output_ready(WaylandOutput const* output) override;
    void output_gone(WaylandOutput const* output) override;

    EGLSurface create_eglsurface(wl_surface* surface, int width, int height);
    void make_current(EGLSurface eglsurface) const;
    void swap_buffers(EGLSurface eglsurface) const;
    void destroy_surface(EGLSurface eglsurface) const;
    void get_surface_size(EGLSurface eglsurface, int* width, int* height) const;
    void set_swap_interval(EGLSurface eglsurface, int interval) const;
    bool supports_surfaceless_context();

    ~MirEglApp();

    std::set<wl_output*> outputs;

private:
    EGLDisplay egldisplay;
    EGLContext eglctx;
    EGLConfig eglconfig;
    EGLint neglconfigs;
    EGLSurface dummy_surface;
};

std::shared_ptr<MirEglApp> make_mir_eglapp(struct wl_display* display)
{
    return std::make_shared<MirEglApp>(display);
}

std::vector<std::shared_ptr<MirEglSurface>> mir_surface_init(std::shared_ptr<MirEglApp> const& mir_egl_app)
{
    std::vector<std::shared_ptr<MirEglSurface>> result;

    for (auto const& output : mir_egl_app->outputs)
    {
        result.push_back(std::make_shared<MirEglSurface>(mir_egl_app, output));
    }

    return result;
}

MirEglSurface::MirEglSurface(
    std::shared_ptr<MirEglApp> const& mir_egl_app,
    struct wl_output* wl_output) :
    mir_egl_app{mir_egl_app}
{
    static struct wl_shell_surface_listener const shell_surface_listener = {
        shell_surface_ping,
        shell_surface_configure,
        shell_surface_popup_done
    };

    display = mir_egl_app->display();
    surface = wl_compositor_create_surface(mir_egl_app->compositor());
    window = wl_shell_get_shell_surface(mir_egl_app->shell(), surface);
    wl_shell_surface_add_listener(window, &shell_surface_listener, this);
    wl_shell_surface_set_fullscreen(window, WL_SHELL_SURFACE_FULLSCREEN_METHOD_SCALE, 0, wl_output);
    wl_surface_commit(surface);
    wl_display_roundtrip(display); // get initial configure

    eglsurface = mir_egl_app->create_eglsurface(surface, width(), height());
    mir_egl_app->set_swap_interval(eglsurface, -1);
}

MirEglSurface::~MirEglSurface()
{
    mir_egl_app->destroy_surface(eglsurface);
    wl_surface_destroy(surface);
    wl_shell_surface_destroy(window);
}

void MirEglSurface::egl_make_current()
{
    mir_egl_app->get_surface_size(eglsurface, &width_, &height_);
    mir_egl_app->make_current(eglsurface);
}

void MirEglSurface::swap_buffers()
{
    mir_egl_app->swap_buffers(eglsurface);
}

unsigned int MirEglSurface::width() const
{
    return width_;
}

unsigned int MirEglSurface::height() const
{
    return height_;
}

void MirEglSurface::shell_surface_ping(void */*data*/, struct wl_shell_surface *wl_shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(wl_shell_surface, serial);
}

void MirEglSurface::shell_surface_configure(void *data,
    struct wl_shell_surface */*wl_shell_surface*/,
    uint32_t /*edges*/,
    int32_t width,
    int32_t height)
{
    auto self = static_cast<MirEglSurface*>(data);
    if (width) self->width_ = width;
    if (height) self->height_ = height;
}

void MirEglSurface::shell_surface_popup_done(void */*data*/, struct wl_shell_surface */*wl_shell_surface*/)
{
}

MirEglApp::MirEglApp(wl_display* display) :
    dummy_surface{EGL_NO_SURFACE}
{
    wayland_init(display);

    unsigned int bpp = 32; //8*MIR_BYTES_PER_PIXEL(pixel_format);

    EGLint attribs[] =
        {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
            EGL_BUFFER_SIZE, (EGLint) bpp,
            EGL_NONE
        };

    egldisplay = eglGetDisplay((EGLNativeDisplayType)(display));
    if (egldisplay == EGL_NO_DISPLAY)
        throw std::runtime_error("Can't eglGetDisplay");

    EGLint major;
    EGLint minor;
    if (!eglInitialize(egldisplay, &major, &minor))
        throw std::runtime_error("Can't eglInitialize");

    if (major != 1 || minor < 4)
        throw std::runtime_error("EGL version is not at least 1.4");

    if (!eglChooseConfig(egldisplay, attribs, &eglconfig, 1, &neglconfigs))
        throw std::runtime_error("Could not eglChooseConfig");

    if (neglconfigs == 0)
        throw std::runtime_error("No EGL config available");

    EGLint ctxattribs[] =
        {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT, ctxattribs);
    if (eglctx == EGL_NO_CONTEXT)
        throw std::runtime_error("eglCreateContext failed");

    if (!supports_surfaceless_context())
    {
        static EGLint const dummy_pbuffer_attribs[] =
        {
             EGL_WIDTH, 1,
             EGL_HEIGHT, 1,
             EGL_NONE
        };

        dummy_surface = eglCreatePbufferSurface(egldisplay, eglconfig, dummy_pbuffer_attribs);
        if (dummy_surface == EGL_NO_SURFACE)
            throw std::runtime_error("eglCreatePbufferSurface failed");
    }

    make_current(dummy_surface);
}

void MirEglApp::output_ready(WaylandOutput const* output)
{
    outputs.insert(output->wl());
}

void MirEglApp::output_gone(WaylandOutput const* output)
{
    outputs.erase(output->wl());
}

EGLSurface MirEglApp::create_eglsurface(wl_surface* surface, int width, int height)
{
    auto const eglsurface = eglCreateWindowSurface(
        egldisplay,
        eglconfig,
        (EGLNativeWindowType)wl_egl_window_create(surface, width, height), NULL);

    if (eglsurface == EGL_NO_SURFACE)
        throw std::runtime_error("eglCreateWindowSurface failed");

    return eglsurface;
}

void MirEglApp::make_current(EGLSurface eglsurface) const
{
    if (!eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx))
        throw std::runtime_error("Can't eglMakeCurrent");
}

void MirEglApp::swap_buffers(EGLSurface eglsurface) const
{
    eglSwapBuffers(egldisplay, eglsurface);
}

void MirEglApp::destroy_surface(EGLSurface eglsurface) const
{
    eglDestroySurface(egldisplay, eglsurface);
}

void MirEglApp::get_surface_size(EGLSurface eglsurface, int* width, int* height) const
{
    eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, width);
    eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, height);
}

void MirEglApp::set_swap_interval(EGLSurface eglsurface, int interval) const
{
    auto const previous_surface = eglGetCurrentSurface(EGL_DRAW);

    make_current(eglsurface);
    eglSwapInterval(egldisplay, interval);

    if (previous_surface != EGL_NO_SURFACE)
        make_current(previous_surface);
}

bool MirEglApp::supports_surfaceless_context()
{
    auto const extensions = eglQueryString(egldisplay, EGL_EXTENSIONS);
    if (!extensions)
        return false;
    return std::strstr(extensions, "EGL_KHR_surfaceless_context") != nullptr;
}

MirEglApp::~MirEglApp()
{
    eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (dummy_surface != EGL_NO_SURFACE)
        destroy_surface(dummy_surface);
    eglDestroyContext(egldisplay, eglctx);
    eglTerminate(egldisplay);
}
