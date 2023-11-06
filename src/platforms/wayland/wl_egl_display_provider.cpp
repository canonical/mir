#include "wl_egl_display_provider.h"

#include "mir/graphics/egl_error.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/platform.h"
#include <EGL/egl.h>
#include <wayland-egl-core.h>

#include <optional>

namespace mg = mir::graphics;
namespace mgw = mir::graphics::wayland;
namespace geom = mir::geometry;

class mgw::WlDisplayAllocator::Framebuffer::EGLState
{
public:
    EGLState(EGLDisplay dpy, EGLContext ctx, EGLSurface surf)
        : dpy{dpy},
          ctx{ctx},
          surf{surf}
    {
    }

    ~EGLState()
    {
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(dpy, ctx);
        eglDestroySurface(dpy, surf);
    }
    
    EGLDisplay const dpy;
    EGLContext const ctx;
    EGLSurface const surf;
};

mgw::WlDisplayAllocator::Framebuffer::Framebuffer(EGLDisplay dpy, EGLContext ctx, EGLSurface surf, geom::Size size)
    : Framebuffer(std::make_shared<EGLState>(dpy, ctx, surf), size)
{
    auto current_ctx = eglGetCurrentContext();
    auto current_draw_surf = eglGetCurrentSurface(EGL_DRAW);
    auto current_read_surf = eglGetCurrentSurface(EGL_READ);

    make_current();
    // Don't block in eglSwapBuffers; we rely on external synchronisation to throttle rendering
    eglSwapInterval(dpy, 0);
    release_current();

    // Paranoia: Restore the previous EGL context state, just in case
    eglMakeCurrent(dpy, current_draw_surf, current_read_surf, current_ctx);
}

mgw::WlDisplayAllocator::Framebuffer::Framebuffer(std::shared_ptr<EGLState const> state, geom::Size size)
    : state{std::move(state)},
      size_{size}
{
}

auto mgw::WlDisplayAllocator::Framebuffer::size() const -> geom::Size
{
    return size_;
}

void mgw::WlDisplayAllocator::Framebuffer::make_current()
{
    if (eglMakeCurrent(state->dpy, state->surf, state->surf, state->ctx) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("eglMakeCurrent failed")));
    }
}

void mgw::WlDisplayAllocator::Framebuffer::release_current()
{
    if (eglMakeCurrent(state->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release EGL context"));
    }
}

void mgw::WlDisplayAllocator::Framebuffer::swap_buffers()
{
    if (eglSwapBuffers(state->dpy, state->surf) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("eglSwapBuffers failed")));
    }
}

auto mgw::WlDisplayAllocator::Framebuffer::clone_handle() -> std::unique_ptr<mg::GenericEGLDisplayAllocator::EGLFramebuffer>
{
    return std::unique_ptr<mg::GenericEGLDisplayAllocator::EGLFramebuffer>{new Framebuffer(state, size_)};
}

mgw::WlDisplayAllocator::WlDisplayAllocator(
    EGLDisplay dpy,
    struct wl_surface* surface,
    geometry::Size size)
    : dpy{dpy},
      wl_window{wl_egl_window_create(surface, size.width.as_int(), size.height.as_int())},
      size{size}
{
}

mgw::WlDisplayAllocator::~WlDisplayAllocator()
{
    wl_egl_window_destroy(wl_window);
}

auto mgw::WlDisplayAllocator::alloc_framebuffer(
    GLConfig const& config,
    EGLContext share_context) -> std::unique_ptr<EGLFramebuffer>
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, config.depth_buffer_bits(),
        EGL_STENCIL_SIZE, config.stencil_buffer_bits(),
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint num_egl_configs;
    EGLConfig egl_config;
    if (eglChooseConfig(dpy, config_attr, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
        num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(egl_error("Failed to choose ARGB EGL config"));
    }

    auto egl_context = eglCreateContext(dpy, egl_config, share_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(egl_error("Failed to create EGL context"));

    auto surf = eglCreatePlatformWindowSurface(dpy, egl_config, wl_window, nullptr);
    if (surf == EGL_NO_SURFACE)
    {
        BOOST_THROW_EXCEPTION(egl_error("Failed to create EGL surface"));
    }

    return std::make_unique<Framebuffer>(
        dpy,
        egl_context,
        surf,
        size);
}

mgw::WlDisplayProvider::WlDisplayProvider(EGLDisplay dpy)
  : dpy{dpy}
{
}

auto mgw::WlDisplayProvider::get_egl_display() -> EGLDisplay
{
    return dpy;
}
