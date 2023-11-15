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

#include "buffer_allocator.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/linux_dmabuf.h"
#include "mir/graphics/dmabuf_buffer.h"
#include "mir/anonymous_shm_file.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/platform.h"
#include "shm_buffer.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/raii.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/context.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/graphics/egl_wayland_allocator.h"
#include "mir/executor.h"
#include "mir/renderer/gl/gl_surface.h"
#include "mir/graphics/display_buffer.h"
#include "kms/egl_helper.h"
#include "mir/graphics/drm_formats.h"
#include "display_helpers.h"
#include "mir/graphics/egl_error.h"
#include "cpu_copy_output_surface.h"
#include "surfaceless_egl_context.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <cassert>
#include <fcntl.h>
#include <xf86drm.h>
#include <drm_fourcc.h>

#define MIR_LOG_COMPONENT "gbm-kms-buffer-allocator"
#include <mir/log.h>
#include <mutex>

namespace mg  = mir::graphics;
namespace mgg = mg::gbm;
namespace mgc = mg::common;
namespace geom = mir::geometry;

mgg::BufferAllocator::BufferAllocator(
    std::unique_ptr<mgg::SurfacelessEGLContext> context,
    std::shared_ptr<mgc::EGLContextExecutor> egl_delegate,
    std::shared_ptr<mg::DMABufEGLProvider> dmabuf_provider)
    : ctx{std::move(context)},
      egl_delegate{std::move(egl_delegate)},
      egl_extensions(std::make_shared<mg::EGLExtensions>()),
      dmabuf_provider{std::move(dmabuf_provider)}
{
}

std::shared_ptr<mg::Buffer> mgg::BufferAllocator::alloc_software_buffer(
    geom::Size size, MirPixelFormat format)
{
    if (!mgc::MemoryBackedShmBuffer::supports(format))
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Trying to create SHM buffer with unsupported pixel format"));
    }

    return std::make_shared<mgc::MemoryBackedShmBuffer>(size, format, egl_delegate);
}

std::vector<MirPixelFormat> mgg::BufferAllocator::supported_pixel_formats()
{
    /*
     * supported_pixel_formats() is kind of a kludge. The right answer depends
     * on whether you're using hardware or software, and it depends on
     * the usage type (e.g. scanout). In the future it's also expected to
     * depend on the GPU model in use at runtime.
     *   To be precise, ShmBuffer now supports OpenGL compositing of all
     * but one MirPixelFormat (bgr_888). But GBM only supports [AX]RGB.
     * So since we don't yet have an adequate API in place to query what the
     * intended usage will be, we need to be conservative and report the
     * intersection of ShmBuffer and GBM's pixel format support. That is
     * just these two. Be aware however you can create a software surface
     * with almost any pixel format and it will also work...
     *   TODO: Convert this to a loop that just queries the intersection of
     * gbm_device_is_format_supported and ShmBuffer::supports(), however not
     * yet while the former is buggy. (FIXME: LP: #1473901)
     */
    static std::vector<MirPixelFormat> const pixel_formats{
        mir_pixel_format_argb_8888,
        mir_pixel_format_xrgb_8888
    };

    return pixel_formats;
}

void mgg::BufferAllocator::bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    try
    {
        mg::wayland::bind_display(dpy, display, *egl_extensions);
        egl_display_bound = true;
    }
    catch (...)
    {
        log(
            logging::Severity::warning,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "Failed to bind EGL Display to Wayland display, falling back to software buffers");
    }

    try
    {
        if (dmabuf_provider)
        {
            mg::EGLExtensions::EXTImageDmaBufImportModifiers modifier_ext{dpy};
            dmabuf_extension =
                std::unique_ptr<LinuxDmaBufUnstable, std::function<void(LinuxDmaBufUnstable*)>>(
                    new LinuxDmaBufUnstable{
                        display,
                        dmabuf_provider,
                    },
                    [wayland_executor](LinuxDmaBufUnstable* global)
                    {
                        // The global must be destroyed on the Wayland thread
                        wayland_executor->spawn(
                            [global]()
                            {
                                /* This is safe against double-frees, as the WaylandExecutor
                                 * guarantees that work scheduled will only run while the Wayland
                                 * event loop is running, and the main loop is stopped before
                                 * wl_display_destroy() frees any globals
                                 *
                                 * This will, however, leak the global if the main loop is destroyed
                                 * before the buffer allocator. Fixing that requires work in the
                                 * wrapper generator.
                                 */
                                delete global;
                            });
                    });
            mir::log_info("Enabled linux-dmabuf import support");
        }
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

    this->wayland_executor = std::move(wayland_executor);
}

void mgg::BufferAllocator::unbind_display(wl_display* display)
{
    if (egl_display_bound)
    {
        auto context_guard = mir::raii::paired_calls(
            [this]() { ctx->make_current(); },
            [this]() { ctx->release_current(); });
        auto dpy = eglGetCurrentDisplay();

        mg::wayland::unbind_display(dpy, display, *egl_extensions);
    }
}

std::shared_ptr<mg::Buffer> mgg::BufferAllocator::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });

    if (auto dmabuf = dmabuf_extension->buffer_from_resource(
        buffer,
        std::function<void()>{on_consumed},
        std::function<void()>{on_release},
        egl_delegate))
    {
        return dmabuf;
    }
    return mg::wayland::buffer_from_resource(
        buffer,
        std::move(on_consumed),
        std::move(on_release),
        *egl_extensions,
        egl_delegate);
}

auto mgg::BufferAllocator::buffer_from_shm(
    std::shared_ptr<renderer::software::RWMappableBuffer> data,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release) -> std::shared_ptr<Buffer>
{
    return std::make_shared<mgc::NotifyingMappableBackedShmBuffer>(
        std::move(data),
        egl_delegate,
        std::move(on_consumed),
        std::move(on_release));
}

auto mgg::BufferAllocator::shared_egl_context() -> EGLContext
{
    return static_cast<EGLContext>(*ctx);
}

auto mgg::GLRenderingProvider::as_texture(std::shared_ptr<Buffer> buffer) -> std::shared_ptr<gl::Texture>
{
    if (auto dmabuf_texture = dmabuf_provider->as_texture(buffer))
    {
        return dmabuf_texture;
    }
    else if (auto tex = std::dynamic_pointer_cast<gl::Texture>(buffer))
    {
        return tex;
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to import buffer as texture; rendering will be incomplete"}));
}

namespace
{
class GBMOutputSurface : public mg::gl::OutputSurface
{
public:
    GBMOutputSurface(
        EGLDisplay dpy,
        EGLContext share_context,
        mg::GLConfig const& config,
        mg::GBMDisplayAllocator& display,
        mg::DRMFormat format,
        mir::geometry::Size size)
        : GBMOutputSurface(
              size,
              dpy,
              create_renderable(dpy, share_context, format, config, display, size))
    {
    }

    ~GBMOutputSurface()
    {
        eglDestroySurface(dpy, egl_surf);
        eglDestroyContext(dpy, ctx);
    }

    void bind() override
    {
        if (!gbm_surface_has_free_buffers(*surface))
        {
            BOOST_THROW_EXCEPTION((std::logic_error{"Attempt to render to GBM surface before releasing previous front buffer"}));
        }
    }

    void make_current() override
    {
        if (eglMakeCurrent(dpy, egl_surf, egl_surf, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make EGL context current"));
        }
    }

    void release_current() override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release EGL context"));
        }
    }

    auto commit() -> std::unique_ptr<mg::Framebuffer> override
    {
        if (eglSwapBuffers(dpy, egl_surf) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("eglSwapBuffers failed"));
        }
        return surface->claim_framebuffer();
    }

    auto size() const -> geom::Size override
    {
        return size_;
    }

    auto layout() const -> Layout override
    {
        return Layout::GL;
    }

private:
    static auto get_matching_configs(EGLDisplay dpy, EGLint const attr[]) -> std::vector<EGLConfig>
    {
        EGLint num_egl_configs;

        // First query the number of matching configs…
        if ((eglChooseConfig(dpy, attr, nullptr, 0, &num_egl_configs) == EGL_FALSE) ||
            (num_egl_configs == 0))
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to enumerate any matching EGL configs"));
        }

        std::vector<EGLConfig> matching_configs(static_cast<size_t>(num_egl_configs));
        if ((eglChooseConfig(dpy, attr, matching_configs.data(), static_cast<EGLint>(matching_configs.size()), &num_egl_configs) == EGL_FALSE) ||
            (num_egl_configs == 0))
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to acquire matching EGL configs"));
        }

        matching_configs.resize(static_cast<size_t>(num_egl_configs));
        return matching_configs;
    }


    static auto egl_config_for_format(EGLDisplay dpy, mg::GLConfig const& config, mg::DRMFormat format) -> std::optional<EGLConfig>
    {
        mg::DRMFormat::RGBComponentInfo const default_components = { 8, 8, 8, 0};

        EGLint const config_attr[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, static_cast<EGLint>(format.components().value_or(default_components).red_bits),
            EGL_GREEN_SIZE, static_cast<EGLint>(format.components().value_or(default_components).green_bits),
            EGL_BLUE_SIZE, static_cast<EGLint>(format.components().value_or(default_components).blue_bits),
            EGL_ALPHA_SIZE, static_cast<EGLint>(format.components().value_or(default_components).alpha_bits.value_or(0)),
            EGL_DEPTH_SIZE, config.depth_buffer_bits(),
            EGL_STENCIL_SIZE, config.stencil_buffer_bits(),
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
        };

        for (auto const& config : get_matching_configs(dpy, config_attr))
        {
            EGLint id;
            if (eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID, &id) == EGL_FALSE)
            {
                mir::log_warning(
                    "Failed to query GBM format of EGLConfig: %s",
                    mg::egl_category().message(eglGetError()).c_str());
                continue;
            }

            if (id == static_cast<EGLint>(format))
            {
                // We've found our matching format, so we're done here.
                return config;
            }
        }
        return std::nullopt;
    }

    static auto create_renderable(
        EGLDisplay dpy,
        EGLContext share_context,
        mg::DRMFormat format,
        mg::GLConfig const& config,
        mg::GBMDisplayAllocator& allocator,
        mir::geometry::Size size)
         -> std::tuple<std::unique_ptr<mg::GBMDisplayAllocator::GBMSurface>, EGLContext, EGLSurface>
    {
        mg::EGLExtensions::PlatformBaseEXT egl_ext;

        auto const [egl_cfg, resolved_format] =
            [&]() -> std::pair<EGLConfig, mg::DRMFormat>
            {
                if (auto eglconfig = egl_config_for_format(dpy, config, format))
                {
                    return std::make_pair(eglconfig.value(), format);
                }

                auto alternate_format =
                    [&]() -> std::optional<mg::DRMFormat>
                    {
                        if (format.has_alpha())
                        {
                            return format.opaque_equivalent();
                        }
                        return format.alpha_equivalent();
                    }();

                if (alternate_format)
                {
                    if (auto eglconfig = egl_config_for_format(dpy, config, *alternate_format))
                    {
                        return std::make_pair(eglconfig.value(), *alternate_format);
                    }
                }

                BOOST_THROW_EXCEPTION((
                    std::runtime_error{
                        std::string{"Failed to find EGL config matching DRM format: "} +
                        format.name()}));
            }();

        auto modifiers = allocator.modifiers_for_format(resolved_format);

        auto surf = allocator.make_surface(size, resolved_format, modifiers);

        auto egl_surf = egl_ext.eglCreatePlatformWindowSurface(
            dpy,
            egl_cfg,
            *surf,
            nullptr);

        if (egl_surf == EGL_NO_SURFACE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL window surface"));
        }


        static const EGLint context_attr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        auto egl_ctx = eglCreateContext(dpy, egl_cfg, share_context, context_attr);
        if (egl_ctx == EGL_NO_CONTEXT)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
        }

        return std::make_tuple(std::move(surf), egl_ctx, egl_surf);
    }

    GBMOutputSurface(
        geom::Size size,
        EGLDisplay dpy,
        std::tuple<std::unique_ptr<mg::GBMDisplayAllocator::GBMSurface>, EGLContext, EGLSurface> renderables)
        : size_{size},
          surface{std::move(std::get<0>(renderables))},
          egl_surf{std::get<2>(renderables)},
          dpy{dpy},
          ctx{std::get<1>(renderables)}
    {
    }

    geom::Size const size_;
    std::unique_ptr<mg::GBMDisplayAllocator::GBMSurface> const surface;
    EGLSurface const egl_surf;
    EGLDisplay const dpy;
    EGLContext const ctx;
};
}

auto mgg::GLRenderingProvider::suitability_for_allocator(
    std::shared_ptr<GraphicBufferAllocator> const& target) -> probe::Result
{
    // TODO: We *can* import from other allocators, maybe (anything with dma-buf is probably possible)
    // For now, the simplest thing is to bind hard to own own allocator.
    if (dynamic_cast<mgg::BufferAllocator*>(target.get()))
    {
        return probe::best;
    }
    return probe::unsupported;
}

auto mgg::GLRenderingProvider::suitability_for_display(
    DisplaySink& sink) -> probe::Result
{
    if (bound_display)
    {
        if (bound_display->on_this_sink(sink))
        {
            /* We're rendering on the same device as display;
             * it doesn't get better than this!
             */
            return probe::best;
        }
    }

    if (sink.acquire_compatible_allocator<CPUAddressableDisplayAllocator>())
    {
        // We *can* render to CPU buffers, but if anyone can do better, let them.
        return probe::supported;
    }

    return probe::unsupported;
}

auto mgg::GLRenderingProvider::surface_for_sink(
    DisplaySink& sink,
    geometry::Size size,
    GLConfig const& config)
    -> std::unique_ptr<gl::OutputSurface>
{
    if (bound_display)
    {
        if (auto gbm_allocator = sink.acquire_compatible_allocator<GBMDisplayAllocator>())
        {
            if (bound_display->on_this_sink(sink))
            {
                return std::make_unique<GBMOutputSurface>(
                    dpy,
                    ctx,
                    config,
                    *gbm_allocator,
                    DRMFormat{DRM_FORMAT_XRGB8888},
                    size);
            }
        }
    }
    auto cpu_allocator = sink.acquire_compatible_allocator<CPUAddressableDisplayAllocator>();

    return std::make_unique<mgc::CPUCopyOutputSurface>(
        dpy,
        ctx,
        *cpu_allocator,
        size);
}

auto mgg::GLRenderingProvider::make_framebuffer_provider(DisplaySink& /*sink*/)
    -> std::unique_ptr<FramebufferProvider>
{
    // TODO: Make this not a null implementation, so bypass/overlays can work again
    class NullFramebufferProvider : public FramebufferProvider
    {
    public:
        auto buffer_to_framebuffer(std::shared_ptr<Buffer>) -> std::unique_ptr<Framebuffer> override
        {
            // It is safe to return nullptr; this will be treated as “this buffer cannot be used as
            // a framebuffer”.
            return {};
        }
    };
    return std::make_unique<NullFramebufferProvider>();
}

mgg::GLRenderingProvider::GLRenderingProvider(
    std::shared_ptr<mg::GBMDisplayProvider> associated_display,
    std::shared_ptr<mgc::EGLContextExecutor> egl_delegate,
    std::shared_ptr<mg::DMABufEGLProvider> dmabuf_provider,
    EGLDisplay dpy,
    EGLContext ctx)
    : bound_display{std::move(associated_display)},
      dpy{dpy},
      ctx{ctx},
      dmabuf_provider{std::move(dmabuf_provider)},
      egl_delegate{std::move(egl_delegate)}
{
}
