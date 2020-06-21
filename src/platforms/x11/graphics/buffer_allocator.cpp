/*
 * Copyright © 2012 Canonical Ltd.
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

#include "buffer_allocator.h"
#include "egl_context_executor.h"
#include "shm_buffer.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/raii.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/context.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/graphics/egl_wayland_allocator.h"
#include "buffer_from_wl_shm.h"
#include "mir/executor.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

#include <algorithm>
#include <stdexcept>
#include <system_error>
#include <cassert>

#include <wayland-server.h>

#define MIR_LOG_COMPONENT "x11-buffer-allocator"
#include <mir/log.h>
#include <mutex>

namespace mg  = mir::graphics;
namespace mgx = mg::X;
namespace mgc = mg::common;
namespace geom = mir::geometry;

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

mgx::BufferAllocator::BufferAllocator(mg::Display const& output)
    : ctx{context_for_output(output)},
      egl_delegate{
          std::make_shared<mgc::EGLContextExecutor>(context_for_output(output))},
      egl_extensions(std::make_shared<mg::EGLExtensions>())
{
}

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::alloc_software_buffer(
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

std::vector<MirPixelFormat> mgx::BufferAllocator::supported_pixel_formats()
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

void mgx::BufferAllocator::bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    mg::wayland::bind_display(dpy, display, *egl_extensions);

    this->wayland_executor = std::move(wayland_executor);
}

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });

    return mg::wayland::buffer_from_resource(
        buffer,
        std::move(on_consumed),
        std::move(on_release),
        ctx,
        *egl_extensions,
        wayland_executor);
}

auto mgx::BufferAllocator::buffer_from_shm(
    wl_resource* buffer,
    std::shared_ptr<Executor> wayland_executor,
    std::function<void()>&& on_consumed) -> std::shared_ptr<Buffer>
{
    return mg::wayland::buffer_from_wl_shm(
        buffer,
        std::move(wayland_executor),
        egl_delegate,
        std::move(on_consumed));
}
