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
#include <wayland-client.h>

#include <cstring>

#include <GLES2/gl2.h>

#include <wayland-egl.h>

#include <chrono>
#include <algorithm>
#include <stdexcept>

static void new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version);

static void global_remove(
    void* data,
    struct wl_registry* registry,
    uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static void output_geometry(void */*data*/,
    struct wl_output */*wl_output*/,
    int32_t /*x*/,
    int32_t /*y*/,
    int32_t /*physical_width*/,
    int32_t /*physical_height*/,
    int32_t /*subpixel*/,
    const char */*make*/,
    const char */*model*/,
    int32_t /*transform*/);

static void output_mode(void *data,
    struct wl_output */*wl_output*/,
    uint32_t /*flags*/,
    int32_t width,
    int32_t height,
    int32_t /*refresh*/);

static void output_done(void* /*data*/, struct wl_output* /*wl_output*/)
{
}

static void output_scale(void* /*data*/, struct wl_output* /*wl_output*/, int32_t /*factor*/)
{
}

class MirEglApp
{
public:
    MirEglApp(struct wl_display* display);

    EGLSurface create_eglsurface(wl_surface* surface, int width, int height);

    void make_current(EGLSurface eglsurface) const;

    void swap_buffers(EGLSurface eglsurface) const;

    void destroy_surface(EGLSurface eglsurface) const;

    void get_surface_size(EGLSurface eglsurface, int* width, int* height) const;

    void set_swap_interval(EGLSurface eglsurface, int interval) const;

    bool supports_surfaceless_context();

    ~MirEglApp();

    struct OutputInfo
    {
        struct wl_output* wl_output;
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    };

    struct wl_display* const display;
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct wl_seat* seat;
    std::vector<OutputInfo> output_info;
    struct wl_shell* shell;

private:
    friend void new_global(
        void* data,
        struct wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t version);

    EGLDisplay egldisplay;
    EGLContext eglctx;
    EGLConfig eglconfig;
    EGLint neglconfigs;
    EGLSurface dummy_surface;
};

static void output_geometry(void *data,
    struct wl_output *wl_output,
    int32_t x,
    int32_t y,
    int32_t /*physical_width*/,
    int32_t /*physical_height*/,
    int32_t /*subpixel*/,
    const char */*make*/,
    const char */*model*/,
    int32_t /*transform*/)
{
    struct MirEglApp* app = static_cast<decltype(app)>(data);

    for (auto& oi : app->output_info)
    {
        if (wl_output == oi.wl_output)
        {
            oi.x = x;
            oi.y = y;
            return;
        }
    }
    app->output_info.push_back({wl_output, x, y, 0, 0});
}

static void output_mode(void *data,
    struct wl_output *wl_output,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t /*refresh*/)
{
    if (!(WL_OUTPUT_MODE_CURRENT & flags))
        return;

    struct MirEglApp* app = static_cast<decltype(app)>(data);

    for (auto& oi : app->output_info)
    {
        if (wl_output == oi.wl_output)
        {
            oi.width = width;
            oi.height = height;
            return;
        }
    }
    app->output_info.push_back({wl_output, 0, 0, width, height});
}

static void new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    (void)version;
    struct MirEglApp* app = static_cast<decltype(app)>(data);

    if (strcmp(interface, "wl_compositor") == 0)
    {
        app->compositor = static_cast<decltype(app->compositor)>(wl_registry_bind(registry, id, &wl_compositor_interface, 3));
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        app->shm = static_cast<decltype(app->shm)>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        app->seat = static_cast<decltype(app->seat)>(wl_registry_bind(registry, id, &wl_seat_interface, 4));
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        wl_output* output = static_cast<decltype(output)>(wl_registry_bind(registry, id, &wl_output_interface, 2));

        app->output_info.push_back({output, 0, 0, 0, 0});
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        app->shell = static_cast<decltype(app->shell)>(wl_registry_bind(registry, id, &wl_shell_interface, 1));
    }
}

std::shared_ptr<MirEglApp> make_mir_eglapp(struct wl_display* display)
{
    return std::make_shared<MirEglApp>(display);
}

std::vector<std::shared_ptr<MirEglSurface>> mir_surface_init(std::shared_ptr<MirEglApp> const& mir_egl_app)
{
    std::vector<std::shared_ptr<MirEglSurface>> result;

    for (auto const& oi : mir_egl_app->output_info)
    {
        result.push_back(std::make_shared<MirEglSurface>(mir_egl_app, oi.wl_output, oi.width, oi.height));
    }

    return result;
}

MirEglSurface::MirEglSurface(
    std::shared_ptr<MirEglApp> const& mir_egl_app,
    struct wl_output* wl_output, int width_, int height_) :
    mir_egl_app{mir_egl_app},
    width_{width_},
    height_{height_}
{
    static struct wl_shell_surface_listener const shell_surface_listener = {
        shell_surface_ping,
        shell_surface_configure,
        shell_surface_popup_done
    };

    display = mir_egl_app->display;
    surface = wl_compositor_create_surface(mir_egl_app->compositor);
    window = wl_shell_get_shell_surface(mir_egl_app->shell, surface);
    wl_shell_surface_add_listener(window, &shell_surface_listener, this);
    wl_shell_surface_set_fullscreen(window, WL_SHELL_SURFACE_FULLSCREEN_METHOD_SCALE, 0, wl_output);

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

MirEglApp::MirEglApp(struct wl_display* display) :
    display{display},
    dummy_surface{EGL_NO_SURFACE}
{
    static struct wl_registry_listener const registry_listener = {
        new_global,
        global_remove
    };

    wl_registry_add_listener(wl_display_get_registry(display), &registry_listener, this);
    wl_display_roundtrip(display);

    static struct wl_output_listener const output_listener = {
        &output_geometry,
        &output_mode,
        &output_done,
        &output_scale,
    };

    for (auto const oi : output_info)
        wl_output_add_listener(oi.wl_output, &output_listener, this);

    wl_display_roundtrip(display);

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
