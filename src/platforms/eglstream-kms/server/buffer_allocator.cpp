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

#include <epoxy/egl.h>
#include <epoxy/gl.h>

#include "buffer_allocator.h"
#include "cpu_copy_output_surface.h"
#include "mir/anonymous_shm_file.h"
#include "mir/graphics/display_sink.h"
#include "mir/graphics/drm_formats.h"
#include "mir/graphics/egl_resources.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/platform.h"
#include "shm_buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/display.h"
#include "wayland-eglstream-controller.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/texture.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/context.h"
#include "mir/raii.h"
#include "mir/wayland/protocol_error.h"
#include "mir/wayland/wayland_base.h"
#include "mir/renderer/gl/gl_surface.h"

#define MIR_LOG_COMPONENT "platform-eglstream-kms"
#include "mir/log.h"

#include <wayland-server-core.h>

#include <mutex>
#include <drm_fourcc.h>

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <cassert>


namespace mg  = mir::graphics;
namespace mge = mg::eglstream;
namespace mgc = mg::common;
namespace geom = mir::geometry;

#ifndef EGL_WL_wayland_eglstream
#define EGL_WL_wayland_eglstream 1
#define EGL_WAYLAND_EGLSTREAM_WL              0x334B
#endif /* EGL_WL_wayland_eglstream */

mge::BufferAllocator::BufferAllocator(std::unique_ptr<renderer::gl::Context> ctx)
    : wayland_ctx{ctx->make_share_context()},
      egl_delegate{
          std::make_shared<mgc::EGLContextExecutor>(std::move(ctx))}
{
}

mge::BufferAllocator::~BufferAllocator() = default;

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_software_buffer(geom::Size size, MirPixelFormat format)
{
    if (!mgc::MemoryBackedShmBuffer::supports(format))
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Trying to create SHM buffer with unsupported pixel format"));
    }

    return std::make_shared<mgc::MemoryBackedShmBuffer>(size, format, egl_delegate);
}

std::vector<MirPixelFormat> mge::BufferAllocator::supported_pixel_formats()
{
    // Lazy
    return {mir_pixel_format_argb_8888, mir_pixel_format_xrgb_8888};
}

namespace
{

GLuint gen_texture_handle()
{
    GLuint tex;
    glGenTextures(1, &tex);
    return tex;
}

struct EGLStreamTextureConsumer
{
    EGLStreamTextureConsumer(std::shared_ptr<mir::renderer::gl::Context> ctx, EGLStreamKHR&& stream)
        : dpy{eglGetCurrentDisplay()},
          stream{std::move(stream)},
          texture{gen_texture_handle()},
          ctx{std::move(ctx)}
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
        // GL_NO_ERROR is 0, so this works.
        if (auto err = glGetError())
        {
            BOOST_THROW_EXCEPTION(mg::gl_error(err, "Failed to bind texture?!"));
        }

        if (eglStreamConsumerGLTextureExternalKHR(dpy, stream) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(
                mg::egl_error("Failed to bind client EGLStream to a texture consumer"));
        }
    }

    ~EGLStreamTextureConsumer()
    {
        bool const need_context = eglGetCurrentContext() == EGL_NO_CONTEXT;
        if (need_context)
        {
            ctx->make_current();
        }
        eglDestroyStreamKHR(dpy, stream);
        glDeleteTextures(1, &texture);
        if (need_context)
        {
            ctx->release_current();
        }
    }

    EGLDisplay const dpy;
    EGLStreamKHR const stream;
    GLuint const texture;
    std::shared_ptr<mir::renderer::gl::Context> const ctx;
};

struct Sync
{
    /*
     * The reserve_sync/set_consumer_sync dance is magical!
     */
    void reserve_sync()
    {
        sync_mutex.lock();
    }

    void set_consumer_sync(GLsync syncpoint)
    {
        if (sync)
        {
            glDeleteSync(sync);
        }
        sync = syncpoint;
        sync_mutex.unlock();
    }

    std::mutex sync_mutex;
    GLsync sync{nullptr};
};

struct BoundEGLStream
{
    static void associate_stream(wl_resource* buffer, std::shared_ptr<mir::renderer::gl::Context> ctx, EGLStreamKHR stream)
    {
        BoundEGLStream* me;
        if (auto notifier = wl_resource_get_destroy_listener(buffer, &on_buffer_destroyed))
        {
            /* We're associating a buffer which has an existing stream with a new stream?
             * The protocol is unclear whether this is an expected behaviour, but the obvious
             * thing to do is to destroy the old stream and associate the new one.
             */
            me = wl_container_of(notifier, me, destruction_listener);
        }
        else
        {
            me = new BoundEGLStream;
            me->destruction_listener.notify = &on_buffer_destroyed;
            wl_resource_add_destroy_listener(buffer, &me->destruction_listener);
        }

        me->producer = std::make_shared<EGLStreamTextureConsumer>(std::move(ctx), std::move(stream));
    }

    class TextureHandle
    {
    public:
        void bind()
        {
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, provider->texture);
        }

        void reserve_sync()
        {
            sync->reserve_sync();
        }

        void set_consumer_sync(GLsync sync)
        {
            this->sync->set_consumer_sync(sync);
        }

        auto tex_id() const -> GLuint
        {
            return provider->texture;
        }

        TextureHandle(TextureHandle&&) = default;
    private:
        friend class BoundEGLStream;

        TextureHandle(
            std::shared_ptr<Sync> syncpoint,
            std::shared_ptr<EGLStreamTextureConsumer const> producer)
            : sync{std::move(syncpoint)},
              provider{std::move(producer)}
        {
            bind();
            /*
             * Isn't this a fun dance!
             *
             * We must insert a glWaitSync here to ensure that the texture is not
             * modified while the commands from the render thread are still executing.
             *
             * We need to lock until *after* eglStreamConsumerAcquireKHR because,
             * once that has executed, it's guaranteed that glBindTexture() in the
             * render thread will bind the new texture (ie: there's some implicit syncpoint
             * action happening)
             */
            std::lock_guard lock{sync->sync_mutex};
            if (sync->sync)
            {
                glWaitSync(sync->sync, 0, GL_TIMEOUT_IGNORED);
                sync->sync = nullptr;
            }
            if (eglStreamConsumerAcquireKHR(provider->dpy, provider->stream) != EGL_TRUE)
            {
                BOOST_THROW_EXCEPTION(
                    mg::egl_error("Failed to latch texture from client EGLStream"));
            }
        }

        TextureHandle(TextureHandle const&) = delete;
        TextureHandle& operator=(TextureHandle const&) = delete;

        std::shared_ptr<Sync> const sync;
        std::shared_ptr<EGLStreamTextureConsumer const> provider;
    };

    static TextureHandle texture_for_buffer(wl_resource* buffer)
    {
        if (auto notifier = wl_resource_get_destroy_listener(buffer, &on_buffer_destroyed))
        {
            BoundEGLStream* me;
            me = wl_container_of(notifier, me, destruction_listener);
            return TextureHandle{me->consumer_sync, me->producer};
        }
        BOOST_THROW_EXCEPTION((std::runtime_error{"Buffer does not have an associated EGLStream"}));
    }
private:
    static void on_buffer_destroyed(wl_listener* listener, void*)
    {
        static_assert(
            std::is_standard_layout<BoundEGLStream>::value,
            "BoundEGLStream must be Standard Layout for wl_container_of to be defined behaviour");

        BoundEGLStream* me;
        me = wl_container_of(listener, me, destruction_listener);
        delete me;
    }

    std::shared_ptr<Sync> const consumer_sync{std::make_shared<Sync>()};
    std::shared_ptr<EGLStreamTextureConsumer const> producer;
    wl_listener destruction_listener;
};
}

void mge::BufferAllocator::create_buffer_eglstream_resource(
    wl_client* client,
    wl_resource* eglstream_controller_resource,
    wl_resource* /*surface*/,
    wl_resource* buffer)
try
{
    auto const allocator = static_cast<mge::BufferAllocator*>(
        wl_resource_get_user_data(eglstream_controller_resource));

    EGLAttrib const attribs[] = {
        EGL_WAYLAND_EGLSTREAM_WL, (EGLAttrib)buffer,
        EGL_NONE
    };

    allocator->wayland_ctx->make_current();
    auto dpy = eglGetCurrentDisplay();

    auto stream = allocator->nv_extensions(dpy).eglCreateStreamAttribNV(dpy, attribs);

    if (stream == EGL_NO_STREAM_KHR)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to create EGLStream from Wayland buffer")));
    }

    BoundEGLStream::associate_stream(buffer, allocator->wayland_ctx, stream);

    allocator->wayland_ctx->release_current();
}
catch (std::exception const& err)
{
    mir::wayland::internal_error_processing_request(client,  "create_buffer_eglstream_resource");
}

struct wl_eglstream_controller_interface const mge::BufferAllocator::impl{
    create_buffer_eglstream_resource
};

void mge::BufferAllocator::bind_eglstream_controller(
    wl_client* client,
    void* ctx,
    uint32_t version,
    uint32_t id)
{
    auto resource = wl_resource_create(client, &wl_eglstream_controller_interface, version, id);

    if (resource == nullptr)
    {
        wl_client_post_no_memory(client);
        mir::log_warning("Failed to create client eglstream-controller resource");
        return;
    }

    wl_resource_set_implementation(
        resource,
        &impl,
        ctx,
        nullptr);
}


void mir::graphics::eglstream::BufferAllocator::bind_display(
    wl_display* display,
    std::shared_ptr<Executor>)
{
    if (!wl_global_create(
        display,
        &wl_eglstream_controller_interface,
        1,
        this,
        &bind_eglstream_controller))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to publish wayland-eglstream-controller global"}));
    }

    auto context_guard = mir::raii::paired_calls(
        [this]() { wayland_ctx->make_current(); },
        [this]() { wayland_ctx->release_current(); });

    auto dpy = eglGetCurrentDisplay();

    if (extensions(dpy).eglBindWaylandDisplayWL(dpy, display) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to bind Wayland EGL display")));
    }

    std::vector<char const*> missing_extensions;
    for (char const* extension : {
        "EGL_KHR_stream_consumer_gltexture",
        "EGL_NV_stream_attrib"})
    {
        if (!epoxy_has_egl_extension(dpy, extension))
        {
            missing_extensions.push_back(extension);
        }
    }

    if (!missing_extensions.empty())
    {
        std::stringstream message;
        message << "Missing required extension" << (missing_extensions.size() > 1 ? "s:" : ":");
        for (auto missing_extension : missing_extensions)
        {
            message << " " << missing_extension;
        }

        BOOST_THROW_EXCEPTION((std::runtime_error{message.str()}));
    }

    mir::log_info("Bound EGLStreams-backed Wayland display");
}

void mir::graphics::eglstream::BufferAllocator::unbind_display(wl_display* display)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { wayland_ctx->make_current(); },
        [this]() { wayland_ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    if (extensions(dpy).eglUnbindWaylandDisplayWL(dpy, display) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to unbind Wayland EGL display")));
    }
}

namespace
{
class EGLStreamBuffer :
    public mg::BufferBasic,
    public mg::NativeBufferBase,
    public mg::gl::Texture
{
public:
    EGLStreamBuffer(
        BoundEGLStream::TextureHandle tex,
        std::function<void()>&& on_consumed,
        MirPixelFormat format,
        geom::Size size,
        Layout layout)
        : size_{size},
          layout_{layout},
          format{format},
          tex{std::move(tex)},
          on_consumed{std::move(on_consumed)}
    {
    }

    mir::geometry::Size size() const override
    {
        return size_;
    }

    MirPixelFormat pixel_format() const override
    {
        return format;
    }

    NativeBufferBase* native_buffer_base() override
    {
        return this;
    }

    mg::gl::Program const& shader(mg::gl::ProgramFactory& cache) const override
    {
        static int shader_id{0};
        return cache.compile_fragment_shader(
            &shader_id,
            "#ifdef GL_ES\n"
            "#extension GL_OES_EGL_image_external : require\n"
            "#endif\n",
            "uniform samplerExternalOES tex;\n"
            "vec4 sample_to_rgba(in vec2 texcoord)\n"
            "{\n"
            "    return texture2D(tex, texcoord);\n"
            "}\n");
    }

    void bind() override
    {
        tex.reserve_sync();
        tex.bind();
    }

    auto tex_id() const -> GLuint override
    {
        return tex.tex_id();
    }

    void add_syncpoint() override
    {
        tex.set_consumer_sync(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
        // TODO: We're going to flush an *awful* lot; try and work out a way to batch this.
        glFlush();
        on_consumed();
    }

    Layout layout() const override
    {
        return layout_;
    }

private:
    mir::geometry::Size const size_;
    Layout const layout_;
    MirPixelFormat const format;
    BoundEGLStream::TextureHandle tex;
    std::function<void()> on_consumed;
};
}

std::shared_ptr<mir::graphics::Buffer>
mir::graphics::eglstream::BufferAllocator::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& /*on_release*/)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { wayland_ctx->make_current(); },
        [this]() { wayland_ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    EGLint width, height;
    if (extensions(dpy).eglQueryWaylandBufferWL(dpy, buffer, EGL_WIDTH, &width) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query Wayland buffer width"));
    }
    if (extensions(dpy).eglQueryWaylandBufferWL(dpy, buffer, EGL_HEIGHT, &height) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query Wayland buffer height"));
    }
    mg::gl::Texture::Layout const layout =
        [&]()
        {
            EGLint y_inverted;
            if (extensions(dpy).eglQueryWaylandBufferWL(dpy, buffer, EGL_WAYLAND_Y_INVERTED_WL, &y_inverted) != EGL_TRUE)
            {
                // If querying Y_INVERTED fails, we must have the default, GL, layout
                return mg::gl::Texture::Layout::GL;
            }
            if (y_inverted)
            {
                return mg::gl::Texture::Layout::GL;
            }
            else
            {
                return mg::gl::Texture::Layout::TopRowFirst;
            }
        }();

    return std::make_shared<EGLStreamBuffer>(
        BoundEGLStream::texture_for_buffer(buffer),
        std::move(on_consumed),
        mir_pixel_format_argb_8888,
        geom::Size{width, height},
        layout);
}

auto mge::BufferAllocator::buffer_from_shm(
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

namespace
{
// libepoxy replaces the GL symbols with resolved-on-first-use function pointers
template<void (**allocator)(GLsizei, GLuint*), void (** deleter)(GLsizei, GLuint const*)>
class GLHandle
{
public:
    GLHandle()
    {
        (**allocator)(1, &id);
    }

    ~GLHandle()
    {
        if (id)
            (**deleter)(1, &id);
    }

    GLHandle(GLHandle const&) = delete;
    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from)
        : id{from.id}
    {
        from.id = 0;
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};

using TextureHandle = GLHandle<&glGenTextures, &glDeleteTextures>;

auto make_stream_ctx(EGLDisplay dpy, EGLConfig cfg, EGLContext share_with) -> EGLContext
{
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint const context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(dpy, cfg, share_with, context_attr);
    if (context == EGL_NO_CONTEXT)
    {
        // One of the ways this will happen is if we try to create one of these
        // on a different device to the display.
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, context) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make EGL context current"));
    }

    return context;
}

auto make_output_surface(EGLDisplay dpy, EGLConfig cfg, EGLStreamKHR output_stream, geom::Size size)
    -> EGLSurface
{
    EGLint const surface_attribs[] = {
        EGL_WIDTH, size.width.as_int(),
        EGL_HEIGHT, size.height.as_int(),
        EGL_NONE,
    };
    auto surface = eglCreateStreamProducerSurfaceKHR(dpy, cfg, output_stream, surface_attribs);
    if (surface == EGL_NO_SURFACE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create StreamProducerSurface"));
    }
    return surface;
}

class EGLStreamOutputSurface : public mg::gl::OutputSurface
{
public:
    EGLStreamOutputSurface(
        EGLDisplay dpy,
        EGLConfig config,
        EGLContext display_share_context,
        EGLStreamKHR output_stream,
        geom::Size size)
        : dpy{dpy},
          ctx{make_stream_ctx(dpy, config, display_share_context)},
          surface{make_output_surface(dpy, config, output_stream, size)},
          size_{std::move(size)}
    {
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    void bind() override
    {
    }

    void make_current() override
    {
        if (eglMakeCurrent(dpy, surface, surface, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
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
        if (eglSwapBuffers(dpy, surface) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("eglSwapBuffers failed"));
        }
        return {};
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
    EGLDisplay const dpy;
    EGLContext const ctx;
    EGLSurface const surface;
    geom::Size const size_;
};

auto pick_stream_surface_config(EGLDisplay dpy, mg::GLConfig const& gl_config) -> EGLConfig
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_STREAM_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, gl_config.depth_buffer_bits(),
        EGL_STENCIL_SIZE, gl_config.stencil_buffer_bits(),
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig egl_config;
    EGLint num_egl_configs{0};

    if (eglChooseConfig(dpy, config_attr, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
        num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose ARGB EGL config"));
    }
    return egl_config;
}
}

mge::GLRenderingProvider::GLRenderingProvider(EGLDisplay dpy, std::unique_ptr<mir::renderer::gl::Context> ctx)
    : dpy{dpy},
      ctx{std::move(ctx)}
{
}

mge::GLRenderingProvider::~GLRenderingProvider() = default;

auto mir::graphics::eglstream::GLRenderingProvider::as_texture(std::shared_ptr<Buffer> buffer)
    -> std::shared_ptr<gl::Texture>
{
    return std::dynamic_pointer_cast<gl::Texture>(std::move(buffer));
}

auto mge::GLRenderingProvider::surface_for_sink(
    DisplaySink& sink,
    mg::GLConfig const& gl_config) -> std::unique_ptr<gl::OutputSurface>
{
    if (auto stream_platform = sink.acquire_compatible_allocator<EGLStreamDisplayAllocator>())
    {
        try
        {
            return std::make_unique<EGLStreamOutputSurface>(
                dpy,
                pick_stream_surface_config(dpy, gl_config),
                static_cast<EGLContext>(*ctx),
                stream_platform->claim_stream(),
                stream_platform->output_size());
        }
        catch (std::exception const& err)
        {
            mir::log_info(
                "Failed to create EGLStream-backed output surface: %s",
                err.what());
        }
    }
    if (auto cpu_provider = sink.acquire_compatible_allocator<CPUAddressableDisplayAllocator>())
    {
        auto fb_context = ctx->make_share_context();
        fb_context->make_current();
        return std::make_unique<mgc::CPUCopyOutputSurface>(
            dpy,
            static_cast<EGLContext>(*ctx),
            *cpu_provider,
            gl_config);
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"DisplayInterfaceProvider does not support any viable output interface"}));
}

auto mge::GLRenderingProvider::suitability_for_allocator(std::shared_ptr<GraphicBufferAllocator> const& target)
    -> probe::Result
{
    // TODO: We *can* import from other allocators, maybe (anything with dma-buf is probably possible)
    // For now, the simplest thing is to bind hard to own own allocator.
    if (dynamic_cast<mge::BufferAllocator*>(target.get()))
    {
        return probe::best;
    }
    return probe::unsupported;
}

auto mge::GLRenderingProvider::suitability_for_display(DisplaySink& sink) -> probe::Result
{
    if (sink.acquire_compatible_allocator<EGLStreamDisplayAllocator>())
    {
        return probe::best;
    }
    if (sink.acquire_compatible_allocator<CPUAddressableDisplayAllocator>())
    {
        return probe::supported;
    }
    return probe::unsupported;
}

auto mge::GLRenderingProvider::make_framebuffer_provider(DisplaySink& /*sink*/)
    -> std::unique_ptr<FramebufferProvider>
{
    // TODO: *Can* we provide overlay support?
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
