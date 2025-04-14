/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rendering_platform.h"
#include "buffer_allocator.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/egl_error.h"
#include "mir/renderer/gl/context.h"

#define MIR_LOG_COMPONENT "platform-generic-egl"
#include "mir/log.h"

#include <EGL/egl.h>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgc = mir::graphics::common;
namespace mge = mg::egl::generic;
namespace geom = mir::geometry;

namespace
{
auto create_default_display() -> EGLDisplay
{
    if (mg::has_egl_client_extension("EGL_EXT_platform_base")
        && mg::has_egl_client_extension("EGL_MESA_platform_surfaceless"))
    {
        // Explicitly create a Surfaceless display, when the extension is supported
        mg::EGLExtensions ext;
        return ext.platform_base->eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
    }
    // Otherwise, hope that the EGL implementation can pull a functional EGLDisplay out of its hat
    return eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

auto egl_display_from_platforms(std::vector<std::shared_ptr<mg::DisplayPlatform>> const& displays) -> std::tuple<EGLDisplay, bool>
{
    for (auto const& display : displays)
    {
        if (auto egl_provider = display->acquire_provider<mg::GenericEGLDisplayProvider>())
        {
            return std::make_tuple(egl_provider->get_egl_display(), false);
        }
    }
    // No Displays provide an EGL display
    // We can still work, falling back to CPU-copy output, as long as we can get *any* EGL display
    auto dpy = create_default_display();
    if (dpy == EGL_NO_DISPLAY)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to create any EGL display"}));
    }
    EGLint major, minor;
    if (eglInitialize(dpy, &major, &minor) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialise EGL"));
    }
    if (std::make_pair(major, minor) < std::make_pair(1, 4))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to get EGL version >= 1.4"}));
    }
    return std::make_tuple(dpy, true);
}

auto make_share_only_context(EGLDisplay dpy, std::optional<EGLContext> share_context) -> EGLContext
{
    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig cfg;
    if (!mg::has_egl_extension(dpy, "EGL_KHR_no_config_context"))
    {
        EGLint num_configs;

        if (eglChooseConfig(dpy, config_attr, &cfg, 1, &num_configs) != EGL_TRUE || num_configs != 1)
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("Failed to find any matching EGL config")));
        }
    }
    else
    {
        cfg = EGL_NO_CONFIG_KHR;
    }

    auto ctx = eglCreateContext(dpy, cfg, share_context.value_or(EGL_NO_CONTEXT), context_attr);
    if (ctx == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }
    return ctx;
}

class SurfacelessEGLContext : public mir::renderer::gl::Context
{
public:
    explicit SurfacelessEGLContext(EGLDisplay dpy)
        : dpy{dpy},
          ctx{make_share_only_context(dpy, {})}
    {
    }

    SurfacelessEGLContext(EGLDisplay dpy, EGLContext share_with)
        : dpy{dpy},
          ctx{make_share_only_context(dpy, share_with)}
    {
    }

    ~SurfacelessEGLContext() override
    {
        eglDestroyContext(dpy, ctx);
    }

    void make_current() const override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
        }
    }

    void release_current() const override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release current EGL context"));
        }
    }

    auto make_share_context() const -> std::unique_ptr<Context> override
    {
        return std::unique_ptr<Context>{new SurfacelessEGLContext{dpy, ctx}};
    }

    explicit operator EGLContext() override
    {
        return ctx;
    }
private:
    EGLDisplay const dpy;
    EGLContext const ctx;
};

auto maybe_make_dmabuf_provider(
    EGLDisplay dpy,
    std::shared_ptr<mg::EGLExtensions> egl_extensions,
    std::shared_ptr<mgc::EGLContextExecutor> egl_delegate)
    -> std::shared_ptr<mg::DMABufEGLProvider>
{
    try
    {
        mg::EGLExtensions::EXTImageDmaBufImportModifiers modifier_ext{dpy};
        return std::make_shared<mg::DMABufEGLProvider>(
            dpy,
            std::move(egl_extensions),
            modifier_ext,
            std::move(egl_delegate),
            [](mg::DRMFormat, std::span<uint64_t const>, geom::Size) -> std::shared_ptr<mg::DMABufBuffer>
            {
                return nullptr;    // We can't (portably) allocate dmabufs, but we also shouldn't need to
            });
    }
    catch (std::runtime_error const& error)
    {
        mir::log_info(
            "Cannot enable linux-dmabuf import support: %s", error.what());
        mir::log(
            mir::logging::Severity::debug,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "Detailed error: ");
    }
    return nullptr;
}
}

mge::RenderingPlatform::RenderingPlatform(std::vector<std::shared_ptr<DisplayPlatform>> const& displays)
    : RenderingPlatform(egl_display_from_platforms(displays))
{
}

mge::RenderingPlatform::RenderingPlatform(std::tuple<EGLDisplay, bool> display)
    : dpy{std::get<0>(display), std::get<1>(display)},
      ctx{std::make_unique<SurfacelessEGLContext>(dpy)},
      dmabuf_provider{
          maybe_make_dmabuf_provider(
              dpy,
              std::make_shared<mg::EGLExtensions>(),
              std::make_shared<mgc::EGLContextExecutor>(ctx->make_share_context()))}
{
}

mge::RenderingPlatform::~RenderingPlatform() = default;

auto mge::RenderingPlatform::create_buffer_allocator(
    mg::Display const& /*output*/) -> mir::UniqueModulePtr<mg::GraphicBufferAllocator>
{
    return make_module_ptr<mge::BufferAllocator>(dpy, static_cast<EGLContext>(*ctx), dmabuf_provider);
}

auto mge::RenderingPlatform::maybe_create_provider(RenderingProvider::Tag const& tag)
    -> std::shared_ptr<RenderingProvider>
{
    if (dynamic_cast<GLRenderingProvider::Tag const*>(&tag))
    {
        return std::make_shared<mge::GLRenderingProvider>(dpy, static_cast<EGLContext>(*ctx), dmabuf_provider);
    }
    return nullptr;
}

mge::RenderingPlatform::EGLDisplayHandle::EGLDisplayHandle(EGLDisplay dpy, bool owns_dpy)
    : dpy{dpy},
      owns_dpy{owns_dpy}
{
}

mge::RenderingPlatform::EGLDisplayHandle::~EGLDisplayHandle()
{
    if (owns_dpy)
    {
        eglTerminate(dpy);
    }
}

mge::RenderingPlatform::EGLDisplayHandle::operator EGLDisplay() const
{
    return dpy;
}
