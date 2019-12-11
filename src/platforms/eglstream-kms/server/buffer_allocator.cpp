/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <epoxy/egl.h>

#include "buffer_allocator.h"
#include "buffer_texture_binder.h"
#include "mir/anonymous_shm_file.h"
#include "shm_buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/display.h"
#include "wayland-eglstream-controller.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/texture.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/renderer/gl/context.h"
#include "mir/raii.h"

#define MIR_LOG_COMPONENT "platform-eglstream-kms"
#include "mir/log.h"

#include <wayland-server-core.h>

#include <mutex>

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <system_error>
#include <cassert>


namespace mg  = mir::graphics;
namespace mge = mg::eglstream;
namespace mgc = mg::common;
namespace geom = mir::geometry;

#ifndef EGL_WL_wayland_eglstream
#define EGL_WL_wayland_eglstream 1
#define EGL_WAYLAND_EGLSTREAM_WL              0x334B
#endif /* EGL_WL_wayland_eglstream */

namespace
{
std::unique_ptr<mir::renderer::gl::Context> context_for_output(mg::Display const& output)
{
    try
    {
        auto& context_source = dynamic_cast<mir::renderer::gl::ContextSource const&>(output);

        /*
         * We care about no part of this context's config; we will do no rendering with it.
         * All we care is that we can allocate texture IDs and bind a texture, which is
         * config independent.
         *
         * That's not *entirely* true; we also need it to be on the same device as we want
         * to do the rendering on, and that GL must support all the extensions we care about,
         * but since we don't yet support heterogeneous hybrid and implementing that will require
         * broader interface changes it's a safe enough requirement for now.
         */
        return context_source.create_gl_context();
    }
    catch (std::bad_cast const& err)
    {
        std::throw_with_nested(
            boost::enable_error_info(
                std::runtime_error{"Output platform cannot provide a GL context"})
                << boost::throw_function(__PRETTY_FUNCTION__)
                << boost::throw_line(__LINE__)
                << boost::throw_file(__FILE__));
    }
}
}

mge::BufferAllocator::BufferAllocator(mg::Display const& output)
    : ctx{context_for_output(output)}
{
}

mge::BufferAllocator::~BufferAllocator() = default;

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_buffer(
    BufferProperties const& buffer_properties)
{
    if (buffer_properties.usage == mg::BufferUsage::software)
        return alloc_software_buffer(buffer_properties.size, buffer_properties.format);
    BOOST_THROW_EXCEPTION(std::runtime_error("platform incapable of creating hardware buffers"));
}

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_software_buffer(geom::Size size, MirPixelFormat format)
{
    if (!mgc::MemoryBackedShmBuffer::supports(format))
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Trying to create SHM buffer with unsupported pixel format"));
    }

    return std::make_shared<mgc::MemoryBackedShmBuffer>(size, format);
}

std::vector<MirPixelFormat> mge::BufferAllocator::supported_pixel_formats()
{
    // Lazy
    return {mir_pixel_format_argb_8888, mir_pixel_format_xrgb_8888};
}

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_buffer(geometry::Size, uint32_t, uint32_t)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("platform incapable of creating buffers"));
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
            std::lock_guard<std::mutex> lock{sync->sync_mutex};
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
    wl_client* /*client*/,
    wl_resource* eglstream_controller_resource,
    wl_resource* /*surface*/,
    wl_resource* buffer)
{
    auto const allocator = static_cast<mge::BufferAllocator*>(
        wl_resource_get_user_data(eglstream_controller_resource));

    EGLAttrib const attribs[] = {
        EGL_WAYLAND_EGLSTREAM_WL, (EGLAttrib)buffer,
        EGL_NONE
    };

    allocator->ctx->make_current();
    auto dpy = eglGetCurrentDisplay();

    auto stream = allocator->nv_extensions.eglCreateStreamAttribNV(dpy, attribs);

    if (stream == EGL_NO_STREAM_KHR)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to create EGLStream from Wayland buffer")));
    }

    BoundEGLStream::associate_stream(buffer, allocator->ctx, stream);

    allocator->ctx->release_current();
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
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });

    auto dpy = eglGetCurrentDisplay();

    if (extensions.eglBindWaylandDisplayWL(dpy, display) != EGL_TRUE)
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

    mir::log_info("Bound EGLStreams-backed WaylandAllocator display");
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
        Layout layout,
        std::unique_ptr<mg::gl::Program>& shader)
        : size_{size},
          layout_{layout},
          format{format},
          tex{std::move(tex)},
          on_consumed{std::move(on_consumed)},
          shader_{shader}
    {
    }

    std::shared_ptr<mir::graphics::NativeBuffer> native_buffer_handle() const override
    {
        return nullptr;
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
        if (!shader_)
        {
            shader_ = cache.compile_fragment_shader(
                "#ifdef GL_ES\n"
                "#extension GL_OES_EGL_image_external : require\n"
                "#endif\n",
                "uniform samplerExternalOES tex;\n"
                "vec4 sample_to_rgba(in vec2 texcoord)\n"
                "{\n"
                "    return texture2D(tex, texcoord);\n"
                "}\n");
        }
        return *shader_;
    }

    void bind() override
    {
        tex.reserve_sync();
        tex.bind();
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
    std::unique_ptr<mg::gl::Program>& shader_;
};
}

std::shared_ptr<mir::graphics::Buffer>
mir::graphics::eglstream::BufferAllocator::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& /*on_release*/)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    EGLint width, height;
    if (extensions.eglQueryWaylandBufferWL(dpy, buffer, EGL_WIDTH, &width) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query Wayland buffer width"));
    }
    if (extensions.eglQueryWaylandBufferWL(dpy, buffer, EGL_HEIGHT, &height) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query Wayland buffer height"));
    }
    mg::gl::Texture::Layout const layout =
        [&]()
        {
            EGLint y_inverted;
            if (extensions.eglQueryWaylandBufferWL(dpy, buffer, EGL_WAYLAND_Y_INVERTED_WL, &y_inverted) != EGL_TRUE)
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
        layout,
        shader);
}
