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

class mgw::WlDisplayProvider::Framebuffer::EGLState
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

mgw::WlDisplayProvider::Framebuffer::Framebuffer(EGLDisplay dpy, EGLContext ctx, EGLSurface surf, geom::Size size)
    : Framebuffer(std::make_shared<EGLState>(dpy, ctx, surf), size)
{
}

mgw::WlDisplayProvider::Framebuffer::Framebuffer(std::shared_ptr<EGLState const> state, geom::Size size)
    : state{std::move(state)},
      size_{size}
{
}

auto mgw::WlDisplayProvider::Framebuffer::size() const -> geom::Size
{
    return size_;
}

void mgw::WlDisplayProvider::Framebuffer::make_current()
{
    if (eglMakeCurrent(state->dpy, state->surf, state->surf, state->ctx) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("eglMakeCurrent failed")));
    }
}

void mgw::WlDisplayProvider::Framebuffer::release_current()
{
    if (eglMakeCurrent(state->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release EGL context"));
    }
}

void mgw::WlDisplayProvider::Framebuffer::swap_buffers()
{
    if (eglSwapBuffers(state->dpy, state->surf) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("eglSwapBuffers failed")));
    }
}

auto mgw::WlDisplayProvider::Framebuffer::clone_handle() -> std::unique_ptr<mg::GenericEGLDisplayProvider::EGLFramebuffer>
{
    return std::unique_ptr<mg::GenericEGLDisplayProvider::EGLFramebuffer>{new Framebuffer(state, size_)};
}

class mgw::WlDisplayProvider::EGLDisplayProvider : public mg::GenericEGLDisplayProvider
{
public:
    EGLDisplayProvider(EGLDisplay dpy);

    EGLDisplayProvider(
        EGLDisplayProvider const& from,
        struct wl_surface* surface,
        geometry::Size size);

    ~EGLDisplayProvider();

    auto get_egl_display() -> EGLDisplay override;

    auto alloc_framebuffer(GLConfig const& config, EGLContext share_context) -> std::unique_ptr<EGLFramebuffer> override;

private:
    EGLDisplay const dpy;

    struct OutputContext
    {
        struct wl_egl_window* wl_window;
        geometry::Size size;
    };
    std::optional<OutputContext> const output;
};

mgw::WlDisplayProvider::EGLDisplayProvider::EGLDisplayProvider(EGLDisplay dpy)
    : dpy{dpy}
{
}

mgw::WlDisplayProvider::EGLDisplayProvider::EGLDisplayProvider(
    EGLDisplayProvider const& from,
    struct wl_surface* surface,
    geometry::Size size)
    : dpy{from.dpy},
      output{
          OutputContext {
              wl_egl_window_create(surface, size.width.as_int(), size.height.as_int()),
              size
      }}
{
}

mgw::WlDisplayProvider::EGLDisplayProvider::~EGLDisplayProvider()
{
    if (output)
    {
        wl_egl_window_destroy(output->wl_window);
    }
}

auto mgw::WlDisplayProvider::EGLDisplayProvider::alloc_framebuffer(
    GLConfig const& config,
    EGLContext share_context) -> std::unique_ptr<EGLFramebuffer>
{
    if (!output)
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"Ooops"}));
    }
    auto const [wl_window, size] = *output;

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

auto mgw::WlDisplayProvider::EGLDisplayProvider::get_egl_display() -> EGLDisplay
{
    return dpy;
}

mgw::WlDisplayProvider::WlDisplayProvider(EGLDisplay dpy)
  : egl_provider{std::make_shared<EGLDisplayProvider>(dpy)}
{
}

mgw::WlDisplayProvider::WlDisplayProvider(
    WlDisplayProvider const& from,
    struct wl_surface* surface,
    geometry::Size size)
    : egl_provider{std::make_shared<EGLDisplayProvider>(*from.egl_provider, surface, size)}
{       
}

auto mgw::WlDisplayProvider::get_egl_display() const -> EGLDisplay
{
    return egl_provider->get_egl_display();
}

auto mgw::WlDisplayProvider::maybe_create_interface(DisplayInterfaceBase::Tag const& type_tag)
    -> std::shared_ptr<DisplayInterfaceBase>
{
    if (dynamic_cast<GenericEGLDisplayProvider::Tag const*>(&type_tag))
    {
        return egl_provider;
    }
    return nullptr;
}
