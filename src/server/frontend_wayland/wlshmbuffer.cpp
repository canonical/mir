/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *   Alan Griffiths <alan@octopull.co.uk>
 */

#include "wlshmbuffer.h"
#include "wayland_executor.h"

#include <mir/log.h>

#include <wayland-server-protocol.h>

#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

#include <boost/throw_exception.hpp>

#include <cstring>

namespace
{
wl_shm_buffer* shm_buffer_from_resource_checked(wl_resource* resource)
{
    auto const buffer = wl_shm_buffer_get(resource);
    if (!buffer)
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"Tried to create WlShmBuffer from non-shm resource"}));
    }

    return buffer;
}

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

bool get_gl_pixel_format(
    MirPixelFormat mir_format,
    GLenum& gl_format,
    GLenum& gl_type)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    GLenum const argb = GL_BGRA_EXT;
    GLenum const abgr = GL_RGBA;
#elif __BYTE_ORDER == __BIG_ENDIAN
    // TODO: Big endian support
GLenum const argb = GL_INVALID_ENUM;
GLenum const abgr = GL_INVALID_ENUM;
//GLenum const rgba = GL_RGBA;
//GLenum const bgra = GL_BGRA_EXT;
#endif

    static const struct
    {
        MirPixelFormat mir_format;
        GLenum gl_format, gl_type;
    } mapping[mir_pixel_formats] =
        {
            {mir_pixel_format_invalid,   GL_INVALID_ENUM, GL_INVALID_ENUM},
            {mir_pixel_format_abgr_8888, abgr,            GL_UNSIGNED_BYTE},
            {mir_pixel_format_xbgr_8888, abgr,            GL_UNSIGNED_BYTE},
            {mir_pixel_format_argb_8888, argb,            GL_UNSIGNED_BYTE},
            {mir_pixel_format_xrgb_8888, argb,            GL_UNSIGNED_BYTE},
            {mir_pixel_format_bgr_888,   GL_INVALID_ENUM, GL_INVALID_ENUM},
            {mir_pixel_format_rgb_888,   GL_RGB,          GL_UNSIGNED_BYTE},
            {mir_pixel_format_rgb_565,   GL_RGB,          GL_UNSIGNED_SHORT_5_6_5},
            {mir_pixel_format_rgba_5551, GL_RGBA,         GL_UNSIGNED_SHORT_5_5_5_1},
            {mir_pixel_format_rgba_4444, GL_RGBA,         GL_UNSIGNED_SHORT_4_4_4_4},
        };

    if (mir_format > mir_pixel_format_invalid &&
        mir_format < mir_pixel_formats &&
        mapping[mir_format].mir_format == mir_format) // just a sanity check
    {
        gl_format = mapping[mir_format].gl_format;
        gl_type = mapping[mir_format].gl_type;
    }
    else
    {
        gl_format = GL_INVALID_ENUM;
        gl_type = GL_INVALID_ENUM;
    }

    return gl_format != GL_INVALID_ENUM && gl_type != GL_INVALID_ENUM;
}
}

namespace mf = mir::frontend;
namespace mg = mir::graphics;
using namespace mir::geometry;

mf::WlShmBuffer::~WlShmBuffer()
{
    executor->spawn([wayland = wayland]()
        {
            std::lock_guard <std::mutex> lock{wayland->mutex};
            if (wayland->resource) {
                wl_resource_queue_event(wayland->resource.value(), WL_BUFFER_RELEASE);
            }
        });
}

std::shared_ptr<mg::Buffer> mf::WlShmBuffer::mir_buffer_from_wl_buffer(
    wl_resource *buffer,
    std::shared_ptr<Executor> executor,
    std::function<void()> &&on_consumed)
{
    DestructionShim* shim = nullptr;

    if (auto notifier = wl_resource_get_destroy_listener(buffer, &on_buffer_destroyed))
    {
        // We've already constructed a shim for this buffer, update it.
        shim = wl_container_of(notifier, shim, destruction_listener);

        if (auto mir_buffer = shim->mir_buffer.lock())
        {
            // There's already a Mir buffer for this wl_buffer
            // Add the new on_consumed, and we're ready to go
            mir_buffer->on_consumed = [a = mir_buffer->on_consumed, b = on_consumed]()
                {
                    a();
                    b();
                };
            return mir_buffer;
        }
        else if (auto resources = shim->resources.lock())
        {
            // There's not a Mir buffer, but there used to be
            // The resources are still alive, which means the destructor's spawn() is still inflight
            // We null out the resources so it will not destroy them when it runs

            std::lock_guard <std::mutex> lock{resources->mutex}; // possibly not necessary, but can't hurt
            resources->buffer = std::experimental::nullopt;
            resources->resource = std::experimental::nullopt;
        }
    }
    else
    {
        shim = new DestructionShim{buffer};
    }

    auto mir_buffer = std::make_shared<WlShmBuffer>(buffer, executor, std::move(on_consumed));
    shim->mir_buffer = mir_buffer;
    shim->resources = mir_buffer->wayland;
    return mir_buffer;
}

std::shared_ptr <mg::NativeBuffer> mf::WlShmBuffer::native_buffer_handle() const
{
    return nullptr;
}

Size mf::WlShmBuffer::size() const
{
    return size_;
}

MirPixelFormat mf::WlShmBuffer::pixel_format() const
{
    return format_;
}

mg::NativeBufferBase* mf::WlShmBuffer::native_buffer_base()
{
    return this;
}

void mf::WlShmBuffer::gl_bind_to_texture()
{
    GLenum format, type;

    if (get_gl_pixel_format(
        format_,
        format,
        type)) {
        /*
         * All existing Mir logic assumes that strides are whole multiples of
         * pixels. And OpenGL defaults to expecting strides are multiples of
         * 4 bytes. These assumptions used to be compatible when we only had
         * 4-byte pixels but now we support 2/3-byte pixels we need to be more
         * careful...
         */
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        read(
            [this, format, type](unsigned char const *pixels)
            {
                auto const size = this->size();
                glTexImage2D(GL_TEXTURE_2D, 0, format,
                             size.width.as_int(), size.height.as_int(),
                             0, format, type, pixels);
            });
    }
}

void mf::WlShmBuffer::bind()
{
    gl_bind_to_texture();
}

void mf::WlShmBuffer::secure_for_render()
{
}

void mf::WlShmBuffer::write(unsigned char const *pixels, size_t size)
{
    std::lock_guard <std::mutex> lock{wayland->mutex};
    if (!wayland->buffer) {
        log_warning("Attempt to write to WlShmBuffer after the wl_buffer has been destroyed");
        return;
    }

    wl_shm_buffer_begin_access(wayland->buffer.value());
    auto data = wl_shm_buffer_get_data(wayland->buffer.value());
    ::memcpy(data, pixels, size);
    wl_shm_buffer_end_access(wayland->buffer.value());
}

void mf::WlShmBuffer::read(std::function<void(unsigned char const *)> const &do_with_pixels)
{
    std::lock_guard <std::mutex> lock{wayland->mutex};
    if (!consumed)
    {
        on_consumed();
        consumed = true;
    }

    do_with_pixels(static_cast<unsigned char const *>(data.get()));
}

Stride mf::WlShmBuffer::stride() const
{
    return stride_;
}

mf::WlShmBuffer::WaylandResources::WaylandResources(wl_resource *resource)
    : resource{resource},
      buffer{shm_buffer_from_resource_checked(resource)}
{
}

mf::WlShmBuffer::DestructionShim::DestructionShim(wl_resource* buffer_resource)
    : destruction_listener{{nullptr, nullptr}, &on_buffer_destroyed}
{
    wl_resource_add_destroy_listener(buffer_resource, &destruction_listener);
}

mf::WlShmBuffer::WlShmBuffer(
    wl_resource *buffer,
    std::shared_ptr<Executor> executor,
    std::function<void()> &&on_consumed)
    :
    wayland{std::make_shared<WaylandResources>(buffer)},
    size_{
        wl_shm_buffer_get_width(wayland->buffer.value()),
        wl_shm_buffer_get_height(wayland->buffer.value())},
    stride_{wl_shm_buffer_get_stride(wayland->buffer.value())},
    format_{wl_format_to_mir_format(wl_shm_buffer_get_format(wayland->buffer.value()))},
    data{std::make_unique<uint8_t[]>(size_.height.as_int() * stride_.as_int())},
    consumed{false},
    on_consumed{std::move(on_consumed)},
    executor{executor}
{
    wl_shm_buffer_begin_access(wayland->buffer.value());
    std::memcpy(data.get(), wl_shm_buffer_get_data(wayland->buffer.value()), size_.height.as_int() * stride_.as_int());
    wl_shm_buffer_end_access(wayland->buffer.value());
}

void mf::WlShmBuffer::on_buffer_destroyed(wl_listener *listener, void *)
{
    static_assert(
        std::is_standard_layout<DestructionShim>::value,
        "DestructionShim must be Standard Layout for wl_container_of to be defined behaviour");

    DestructionShim *shim;
    shim = wl_container_of(listener, shim, destruction_listener);

    {
        if (auto resources = shim->resources.lock())
        {
            std::lock_guard <std::mutex> lock{resources->mutex};
            resources->buffer = std::experimental::nullopt;
            resources->resource = std::experimental::nullopt;
        }
    }

    delete shim;
}
