#include "wl_egl_display_provider.h"

#include "mir/graphics/egl_error.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/platform.h"
#include <EGL/egl.h>
#include <wayland-egl-core.h>

#include <mutex>
#include <optional>

namespace mg = mir::graphics;
namespace mgw = mir::graphics::wayland;
namespace geom = mir::geometry;

namespace
{
// Utility to restore EGL state on scope exit
class CacheEglState
{
public:
    CacheEglState() = default;

    ~CacheEglState()
    {
        eglMakeCurrent(dpy, draw_surf, read_surf, ctx);
    }
private:
    CacheEglState(CacheEglState const&) = delete;
    EGLDisplay const dpy = eglGetCurrentDisplay();
    EGLContext const ctx = eglGetCurrentContext();
    EGLSurface const draw_surf = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface const read_surf = eglGetCurrentSurface(EGL_READ);
};
}

class mgw::WlDisplayAllocator::SurfaceState
{
public:
    SurfaceState(EGLDisplay dpy, struct ::wl_egl_window* wl_window) :
        dpy{dpy}, wl_window{wl_window} {}

    ~SurfaceState()
    {
        if (egl_surface != EGL_NO_SURFACE) eglDestroySurface(dpy, egl_surface);
        wl_egl_window_destroy(wl_window);
    }

    auto surface(EGLConfig egl_config) -> EGLSurface
    {
        std::lock_guard lock{mutex};
        if (egl_surface == EGL_NO_SURFACE)
        {
            egl_surface = eglCreatePlatformWindowSurface(dpy, egl_config, wl_window, nullptr);
        }

        if (egl_surface == EGL_NO_SURFACE)
        {
            BOOST_THROW_EXCEPTION(egl_error("Failed to create EGL surface"));
        }

        return egl_surface;
    }

private:
    EGLDisplay const dpy;
    struct ::wl_egl_window* const wl_window;

    std::mutex mutex;
    EGLSurface egl_surface{EGL_NO_SURFACE};
};

class mgw::WlDisplayAllocator::Buffer::EGLState
{
public:
    EGLState(EGLDisplay dpy, EGLContext ctx, EGLSurface surf, std::shared_ptr<SurfaceState> ss)
        : dpy{dpy},
          ctx{ctx},
          surf{surf},
          ss{std::move(ss)}
    {
        CacheEglState stash;

        make_current();
        // Don't block in eglSwapBuffers; we rely on external synchronisation to throttle rendering
        eglSwapInterval(dpy, 0);
        release_current();
    }

    ~EGLState()
    {
        if (ctx == eglGetCurrentContext())
        {
            eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
        eglDestroyContext(dpy, ctx);
    }

    void make_current() const
    {
        if (eglMakeCurrent(dpy, surf, surf, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("eglMakeCurrent failed")));
        }
    }

    void release_current() const
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release EGL context"));
        }
    }

    void swap_buffers() const
    {
        if (eglSwapBuffers(dpy, surf) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("eglSwapBuffers failed")));
        }
    }
    
    MirPixelFormat format() const
    {
        EGLint texture_format = -1;
        eglQuerySurface(dpy, surf, EGL_TEXTURE_FORMAT, &texture_format);
        
        switch(texture_format)
        {
            case EGL_TEXTURE_RGB:
                return mir_pixel_format_rgb_888;
            case EGL_TEXTURE_RGBA:
                return mir_pixel_format_argb_8888;
            default:
                return mir_pixel_format_invalid;
        }
    }

    EGLDisplay const dpy;
    EGLContext const ctx;
    EGLSurface const surf;
    std::shared_ptr<SurfaceState> const ss;
};

mgw::WlDisplayAllocator::Buffer::Buffer(
    EGLDisplay dpy, EGLContext ctx, EGLSurface surf, std::shared_ptr<SurfaceState> ss, mir::geometry::Size size) :
    Buffer{std::make_shared<EGLState>(dpy, ctx, surf, ss), size}
{
}

mgw::WlDisplayAllocator::Buffer::Buffer(std::shared_ptr<EGLState const> state, geom::Size size)
    : state{std::move(state)}, size_{size}
{
}

mgw::WlDisplayAllocator::Buffer::Buffer(Buffer const& that)
    : state{that.state},
      size_{that.size_}
{
}

auto mgw::WlDisplayAllocator::Buffer::id() const -> BufferID
{
    return BufferID{static_cast<uint32_t>(reinterpret_cast<uintptr_t>(state->surf))};
}

auto mgw::WlDisplayAllocator::Buffer::size() const -> geom::Size
{
    return size_;
}

auto mgw::WlDisplayAllocator::Buffer::pixel_format() const -> MirPixelFormat
{
    return state->format();
}

void mgw::WlDisplayAllocator::Buffer::make_current()
{
    state->make_current();
}

void mgw::WlDisplayAllocator::Buffer::release_current()
{
    state->release_current();
}

void mgw::WlDisplayAllocator::Buffer::swap_buffers()
{
    state->swap_buffers();
}

auto mgw::WlDisplayAllocator::Buffer::clone_handle() -> std::unique_ptr<mg::GenericEGLDisplayAllocator::EGLBuffer>
{
    return std::unique_ptr<mg::GenericEGLDisplayAllocator::EGLBuffer>{new Buffer(*this)};
}

auto mgw::WlDisplayAllocator::Buffer::to_framebuffer() -> std::unique_ptr<mg::Framebuffer>
{
    return std::make_unique<mgw::WlDisplayAllocator::Framebuffer>(new Buffer(*this));
}

mgw::WlDisplayAllocator::WlDisplayAllocator(
    EGLDisplay dpy,
    struct wl_surface* surface,
    geometry::Size size)
    : dpy{dpy},
      surface_state{std::make_shared<SurfaceState>(dpy, wl_egl_window_create(surface, size.width.as_int(), size.height.as_int()))},
      size{size}
{
}

mgw::WlDisplayAllocator::~WlDisplayAllocator()
{
}

auto mgw::WlDisplayAllocator::alloc_buffer(
    GLConfig const& config,
    EGLContext share_context) -> std::unique_ptr<EGLBuffer>
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
    {
        BOOST_THROW_EXCEPTION(egl_error("Failed to create EGL context"));
    }

    auto surface = surface_state->surface(egl_config);

    return std::make_unique<Buffer>(dpy, egl_context, surface, surface_state, size);
}

mgw::WlDisplayProvider::WlDisplayProvider(EGLDisplay dpy)
  : dpy{dpy}
{
}

auto mgw::WlDisplayProvider::get_egl_display() -> EGLDisplay
{
    return dpy;
}
