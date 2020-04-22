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
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
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
#include "egl_context_executor.h"
#include "mir/executor.h"
#include "shm_buffer.h"
#include "buffer_allocator.h"
#include "buffer_from_wl_shm.h"

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

auto mg::rpi::BufferAllocator::alloc_buffer(mg::BufferProperties const&)
    -> std::shared_ptr<Buffer>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"Platform does not support deprecated alloc_buffer()"}));
}

auto mg::rpi::BufferAllocator::supported_pixel_formats()
    -> std::vector<MirPixelFormat>
{
    return {mir_pixel_format_argb_8888};
}

auto mg::rpi::BufferAllocator::alloc_buffer(
    mir::geometry::Size, uint32_t, uint32_t)
    -> std::shared_ptr<Buffer>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"Platform does not support mirclient"}));
}

namespace
{

class DispmanxShmBuffer
    : public mg::common::ShmBuffer,
      public mir::renderer::software::PixelSource,
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

    auto native_buffer_handle() const -> std::shared_ptr<mg::NativeBuffer> override
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"mirclient not supported"}));
    }

    void write(unsigned char const* pixels, size_t /*size*/) override
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
            const_cast<unsigned char*>(pixels),
            &rect);
    }

    void read(std::function<void(unsigned char const*)> const& do_with_pixels) override
    {
        auto const width = size().width.as_int();
        auto const height = size().height.as_int();

        VC_RECT_T const rect{0, 0, width, height};

        auto const bounce_buffer = std::make_unique<unsigned char[]>(
            stride().as_uint32_t() * height);

        vc_dispmanx_resource_read_data(
            static_cast<DISPMANX_RESOURCE_HANDLE_T>(*this),
            &rect,
            bounce_buffer.get(),
            stride().as_uint32_t());

        do_with_pixels(bounce_buffer.get());
    }

    mir::geometry::Stride stride() const override
    {
        return stride_;
    }

    void bind() override
    {
        ShmBuffer::bind();

        /*
         * Slowpath: we download from VideoCore memory before uploading again
         */
        read([this](auto pixels) { upload_to_texture(pixels, stride()); });
    }

    explicit operator DISPMANX_RESOURCE_HANDLE_T() const override
    {
        return handle;
    }
private:
    geom::Stride const stride_;
    DISPMANX_RESOURCE_HANDLE_T const handle;
};
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
        geom::Stride{MIR_BYTES_PER_PIXEL(format) * size.width.as_uint32_t()},
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

    auto native_buffer_handle() const -> std::shared_ptr<mg::NativeBuffer> override
    {
        return {};
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

            on_consumed();
            uploaded = true;
        }
    }

    void add_syncpoint() override
    {

    }

    explicit operator DISPMANX_RESOURCE_HANDLE_T() const override
    {
        return vc_dispmanx_get_handle_from_wl_buffer(buffer);
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

    if (!egl_extensions->wayland)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"No EGL_WL_bind_wayland_display support"}));
    }

    if (egl_extensions->wayland->eglBindWaylandDisplayWL(dpy, display) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to bind Wayland EGL display"));
    }
    else
    {
        mir::log_info("Bound WaylandAllocator display");
    }
    this->wayland_executor = std::move(wayland_executor);
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

namespace
{
MirPixelFormat wl_format_to_mir_format(uint32_t format)
{
    switch (format)
    {
        case WL_SHM_FORMAT_ARGB8888:
            return mir_pixel_format_argb_8888;
        case WL_SHM_FORMAT_XRGB8888:
            return mir_pixel_format_xrgb_8888;
        case WL_SHM_FORMAT_RGBA4444:
            return mir_pixel_format_rgba_4444;
        case WL_SHM_FORMAT_RGBA5551:
            return mir_pixel_format_rgba_5551;
        case WL_SHM_FORMAT_RGB565:
            return mir_pixel_format_rgb_565;
        case WL_SHM_FORMAT_RGB888:
            return mir_pixel_format_rgb_888;
        case WL_SHM_FORMAT_BGR888:
            return mir_pixel_format_bgr_888;
        case WL_SHM_FORMAT_XBGR8888:
            return mir_pixel_format_xbgr_8888;
        case WL_SHM_FORMAT_ABGR8888:
            return mir_pixel_format_abgr_8888;
        default:
            return mir_pixel_format_invalid;
    }
}

class DispmanxWlShmBuffer : public DispmanxShmBuffer
{
public:
    DispmanxWlShmBuffer(
        wl_shm_buffer* buffer,
        std::shared_ptr<mg::common::EGLContextExecutor> egl_executor,
        std::function<void()>&& on_consumed)
        : DispmanxShmBuffer(
            geom::Size{wl_shm_buffer_get_width(buffer), wl_shm_buffer_get_height(buffer)},
            geom::Stride{wl_shm_buffer_get_stride(buffer)},
            wl_format_to_mir_format(wl_shm_buffer_get_format(buffer)),
            std::move(egl_executor)),
          on_consumed(std::move(on_consumed))
    {
        wl_shm_buffer_begin_access(buffer);
        write(
            static_cast<unsigned char*>(wl_shm_buffer_get_data(buffer)),
            stride().as_uint32_t() * size().height.as_uint32_t());
        wl_shm_buffer_end_access(buffer);
    }

    void bind() override
    {
        ShmBuffer::bind();

        std::lock_guard<std::mutex> lock{consumption_mutex};
        if (on_consumed)
        {
            read([this](auto pixels) { upload_to_texture(pixels, stride()); });
            on_consumed();
            on_consumed = nullptr;
        }
    }

private:
    std::mutex consumption_mutex;
    std::function<void()> on_consumed;
};
}

auto mg::rpi::BufferAllocator::buffer_from_shm(
    wl_resource* buffer,
    std::shared_ptr<mir::Executor> /*wayland_executor*/,
    std::function<void()>&& on_consumed) -> std::shared_ptr<Buffer>
{
    auto shm_buffer = wl_shm_buffer_get(buffer);
    if (shm_buffer == nullptr)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempt to allocate Shm buffer from non-shm wl_resource"}));
    }

    auto const mir_buffer = std::make_shared<DispmanxWlShmBuffer>(
        shm_buffer,
        egl_executor,
        std::move(on_consumed));

    // DispmanxWlShmBuffer eagerly copies out of the wl_shm_buffer, so we're done with it here.
    wl_resource_post_event(buffer, WL_BUFFER_RELEASE);
    return mir_buffer;
}
