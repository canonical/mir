/*
 * Copyright © 2019 Canonical Ltd.
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

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#define BUILD_WAYLAND
#include <interface/vmcs_host/vc_dispmanx.h>
#include <interface/vmcs_host/vc_vchi_dispmanx.h>
#undef BUILD_WAYLAND

#include "helpers.h"
#include "mir/raii.h"
#include "mir/anonymous_shm_file.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir/executor.h"
#include "shm_buffer.h"
#include "buffer_allocator.h"

#define MIR_LOG_COMPONENT "rpi-dispmanx"
#include <mir/log.h>


namespace geom = mir::geometry;
namespace mg = mir::graphics;

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

mg::rpi::BufferAllocator::BufferAllocator(mir::graphics::Display const& output)
    : egl_extensions{std::make_shared<mg::EGLExtensions>()},
      ctx{context_for_output(output)},
      egl_executor{
        std::make_shared<mg::common::EGLContextExecutor>(context_for_output(output))}
{
}

auto mg::rpi::BufferAllocator::supported_pixel_formats()
    -> std::vector<MirPixelFormat>
{
    return {mir_pixel_format_argb_8888};
}

namespace
{

class DispmanxShmBuffer
    : public mg::common::ShmBuffer,
      public mir::renderer::software::ReadTransferableBuffer,
      public mir::renderer::software::WriteTransferableBuffer,
      public mg::rpi::DispmanXBuffer
{
public:
    DispmanxShmBuffer(
        geom::Size const& size,
        geom::Stride const& stride,
        MirPixelFormat format,
        std::shared_ptr<mg::common::EGLContextExecutor> egl_executor)
        : ShmBuffer(size, format, std::move(egl_executor)),
          stride_{stride},
          handle{mg::rpi::dispmanx_resource_for(size, stride_, format)}
    {
    }

    ~DispmanxShmBuffer() override
    {
        vc_dispmanx_resource_delete(handle);
    }

    auto format() const -> MirPixelFormat override
    {
        return pixel_format();
    }

    auto size() const -> mir::geometry::Size override
    {
        return ShmBuffer::size();
    }

    void transfer_from_buffer(unsigned char* destination) const override
    {
        auto const width = size().width.as_int();
        auto const height = size().height.as_int();

        VC_RECT_T const rect{0, 0, width, height};

        vc_dispmanx_resource_read_data(
            static_cast<DISPMANX_RESOURCE_HANDLE_T>(*this),
            &rect,
            destination,
            stride().as_uint32_t());
    }

    void transfer_into_buffer(unsigned char const* source) override
    {
        auto const vc_format = mg::rpi::vc_image_type_from_mir_pf(pixel_format());

        VC_RECT_T rect;
        vc_dispmanx_rect_set(
            &rect,
            0, 0,
            size().width.as_uint32_t(), size().height.as_uint32_t());

        vc_dispmanx_resource_write_data(
            handle,
            vc_format,
            stride().as_uint32_t(),
            const_cast<unsigned char*>(source),
            &rect);
    }

    mir::geometry::Stride stride() const override
    {
        return stride_;
    }

    void bind() override
    {
        ShmBuffer::bind();

        auto const pixels = std::make_unique<unsigned char[]>(stride().as_uint32_t() * size().height.as_uint32_t());
        transfer_from_buffer(pixels.get());

        upload_to_texture(pixels.get(), stride());
    }

    explicit operator DISPMANX_RESOURCE_HANDLE_T() const override
    {
        return handle;
    }

    auto resource_transform() const -> DISPMANX_TRANSFORM_T override
    {
        return DISPMANX_NO_ROTATE;
    }
private:
    geom::Stride const stride_;
    DISPMANX_RESOURCE_HANDLE_T const handle;
};

auto calculate_stride(geom::Size const& size, MirPixelFormat format) -> geom::Stride
{
    auto const minimum_stride = MIR_BYTES_PER_PIXEL(format) * size.width.as_uint32_t();
    // DispmanX likes the stride to be a multiple of 64 bytes.
    if (minimum_stride % 64 == 0)
    {
        return geom::Stride{minimum_stride};
    }
    auto const rounded_stride = minimum_stride + (64 - (minimum_stride % 64));
    return geom::Stride{rounded_stride};
}
}
auto mg::rpi::BufferAllocator::alloc_software_buffer(
    mir::geometry::Size size, MirPixelFormat format)
    -> std::shared_ptr<Buffer>
{
    if (!common::ShmBuffer::supports(format))
    {
        BOOST_THROW_EXCEPTION((
            std::runtime_error{"Trying to create SHM buffer with unsupported pixel format"}));
    }

    return std::make_shared<DispmanxShmBuffer>(
        size,
        calculate_stride(size, format),
        format,
        egl_executor);
}

namespace
{
mir::geometry::Size extract_size_from_buffer(wl_resource* buffer)
{
    auto* const server =
        static_cast<struct wl_dispmanx_server_buffer*>(wl_resource_get_user_data(buffer));

    return mir::geometry::Size(server->width, server->height);
}

class WlDispmanxBuffer :
    public mg::BufferBasic,
    public mg::NativeBufferBase,
    public mg::gl::Texture,
    public mg::rpi::DispmanXBuffer
{
public:
    WlDispmanxBuffer(
        wl_resource* buffer,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release)
        : buffer{buffer},
          size_{extract_size_from_buffer(buffer)},
          on_consumed{std::move(on_consumed)},
          on_release{std::move(on_release)}
    {
        vc_dispmanx_set_wl_buffer_in_use(buffer, 1);
    }

    ~WlDispmanxBuffer() override
    {
        vc_dispmanx_set_wl_buffer_in_use(buffer, 0);
        on_release();
    }

    mir::geometry::Size size() const override
    {
        return size_;
    }

    auto pixel_format() const -> MirPixelFormat override
    {
        auto* const server =
            static_cast<struct wl_dispmanx_server_buffer*>(wl_resource_get_user_data(buffer));

        switch(server->format)
        {
            case WL_DISPMANX_FORMAT_ABGR8888:
                return mir_pixel_format_abgr_8888;
            case WL_DISPMANX_FORMAT_XBGR8888:
                return mir_pixel_format_xbgr_8888;
            case WL_DISPMANX_FORMAT_RGB565:
                return mir_pixel_format_rgb_565;
            default:
                BOOST_THROW_EXCEPTION((std::runtime_error{"Invalid pixel format"}));
        }
    }

    auto native_buffer_base() -> mg::NativeBufferBase* override
    {
        return this;
    }

    auto shader(mg::gl::ProgramFactory& factory) const -> mg::gl::Program const& override
    {
        static int rgba_shader_tag{0};
        return factory.compile_fragment_shader(
            &rgba_shader_tag,
            "",
            "uniform sampler2D tex;\n"
            "vec4 sample_to_rgba(in vec2 texcoord)\n"
            "{\n"
            "    return texture2D(tex, texcoord);\n"
            "}\n");
    }

    auto layout() const -> Layout override
    {
        return Layout::GL;
    }

    void bind() override
    {
        if (!uploaded)
        {
            glGenTextures(1, &tex_id);
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        if (!uploaded)
        {
            auto const width = size().width.as_int();
            auto const height = size().height.as_int();
            auto const stride =
                width * (pixel_format() != mir_pixel_format_rgb_565 ? 4 : 2);
            auto const gl_format =
                (pixel_format() != mir_pixel_format_rgb_565 ? GL_RGBA : GL_RGB);
            auto const gl_type =
                (pixel_format() == mir_pixel_format_rgb_565 ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE);

            VC_RECT_T const rect{0, 0, width, height};

            auto const bounce_buffer = std::make_unique<unsigned char[]>(
                stride * height);

            vc_dispmanx_resource_read_data(
                static_cast<DISPMANX_RESOURCE_HANDLE_T>(*this),
                &rect,
                bounce_buffer.get(),
                stride);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, gl_format, width, height,
                0, gl_format, gl_type, bounce_buffer.get());

            uploaded = true;
        }
    }

    void add_syncpoint() override
    {

    }

    explicit operator DISPMANX_RESOURCE_HANDLE_T() const override
    {
        on_consumed();
        const_cast<WlDispmanxBuffer*>(this)->on_consumed = [](){};
        return vc_dispmanx_get_handle_from_wl_buffer(buffer);
    }

    auto resource_transform() const -> DISPMANX_TRANSFORM_T override
    {
        return static_cast<DISPMANX_TRANSFORM_T>(DISPMANX_NO_ROTATE | DISPMANX_FLIP_VERT);
    }
private:
    wl_resource* buffer;
    mir::geometry::Size const size_;
    bool uploaded{false};
    GLuint tex_id;
    std::function<void()> on_consumed;
    std::function<void()> on_release;
};

}

void mg::rpi::BufferAllocator::bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    if (egl_extensions->wayland(dpy).eglBindWaylandDisplayWL(dpy, display) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to bind Wayland EGL display"));
    }
    else
    {
        mir::log_info("Bound Wayland display");
    }
    this->wayland_executor = std::move(wayland_executor);
}

void mg::rpi::BufferAllocator::unbind_display(wl_display* display)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    if (egl_extensions->wayland(dpy).eglUnbindWaylandDisplayWL(dpy, display) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to unbind Wayland EGL display"));
    }
    else
    {
        mir::log_info("Unbound Wayland display");
    }
}

std::shared_ptr<mg::Buffer> mg::rpi::BufferAllocator::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });

    return std::make_shared<WlDispmanxBuffer>(
        buffer,
        std::move(on_consumed),
        std::move(on_release));
}

class DispmanxWlShmBuffer : public DispmanxShmBuffer
{
public:
    DispmanxWlShmBuffer(
        std::shared_ptr<mir::renderer::software::RWMappableBuffer> shm_data,
        std::shared_ptr<mg::common::EGLContextExecutor> egl_executor,
        std::function<void()>&& on_consumed)
        : DispmanxShmBuffer(
            shm_data->size(),
            shm_data->stride(),
            shm_data->format(),
            std::move(egl_executor)),
          on_consumed(std::move(on_consumed))
    {
        auto mapping = shm_data->map_readable();
        transfer_into_buffer(mapping->data());
    }

    void bind() override
    {
        ShmBuffer::bind();

        std::lock_guard lock{consumption_mutex};
        if (on_consumed)
        {
            auto const pixels = std::make_unique<unsigned char[]>(stride().as_uint32_t() * size().height.as_uint32_t());
            transfer_from_buffer(pixels.get());

            upload_to_texture(pixels.get(), stride());
            on_consumed();
            on_consumed = nullptr;
        }
    }

private:
    std::mutex consumption_mutex;
    std::function<void()> on_consumed;
};

auto mg::rpi::BufferAllocator::buffer_from_shm(
    std::shared_ptr<renderer::software::RWMappableBuffer> shm_data,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release) -> std::shared_ptr<Buffer>
{
    auto const mir_buffer = std::make_shared<DispmanxWlShmBuffer>(
        std::move(shm_data),
        egl_executor,
        std::move(on_consumed));

    // DispmanxWlShmBuffer eagerly copies out of the wl_shm_buffer, so we're done with it here.
    on_release();
    return mir_buffer;
}
