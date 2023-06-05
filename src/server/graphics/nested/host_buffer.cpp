/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "host_buffer.h"
#include "mir_toolkit/mir_buffer_private.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

mgn::HostBuffer::HostBuffer(MirConnection* mir_connection, mg::BufferProperties const& properties) :
    fence_extensions(mir_extension_fenced_buffers_v1(mir_connection))
{
    mir_connection_allocate_buffer(
        mir_connection,
        properties.size.width.as_int(),
        properties.size.height.as_int(),
        properties.format,
        buffer_available, this);
    std::unique_lock<std::mutex> lk(mut);
    cv.wait(lk, [&]{ return handle; });
    if (!mir_buffer_is_valid(handle))
    {
        mir_buffer_release(handle);
        BOOST_THROW_EXCEPTION(std::runtime_error("could not allocate MirBuffer"));
    }
}

mgn::HostBuffer::HostBuffer(MirConnection* mir_connection, geom::Size size, MirPixelFormat format) :
    fence_extensions(mir_extension_fenced_buffers_v1(mir_connection))
{
    mir_connection_allocate_buffer(
        mir_connection,
        size.width.as_int(),
        size.height.as_int(),
        format,
        buffer_available, this);
    std::unique_lock<std::mutex> lk(mut);
    cv.wait(lk, [&]{ return handle; });
    if (!mir_buffer_is_valid(handle))
    {
        mir_buffer_release(handle);
        BOOST_THROW_EXCEPTION(std::runtime_error("could not allocate MirBuffer"));
    }
}

mgn::HostBuffer::HostBuffer(MirConnection* mir_connection,
    MirExtensionGbmBufferV1 const* ext,
    geom::Size size, unsigned int native_pf, unsigned int native_flags) :
    fence_extensions(mir_extension_fenced_buffers_v1(mir_connection))
{
    ext->allocate_buffer_gbm(
        mir_connection, size.width.as_int(), size.height.as_int(), native_pf, native_flags,
        buffer_available, this);
    std::unique_lock<std::mutex> lk(mut);
    cv.wait(lk, [&]{ return handle; });
    if (!mir_buffer_is_valid(handle))
    {
        mir_buffer_release(handle);
        BOOST_THROW_EXCEPTION(std::runtime_error("could not allocate MirBuffer"));
    }
}

mgn::HostBuffer::HostBuffer(MirConnection* mir_connection,
    MirExtensionAndroidBufferV1 const* ext,
    geom::Size size, unsigned int native_pf, unsigned int native_flags) :
    fence_extensions(mir_extension_fenced_buffers_v1(mir_connection))
{
    ext->allocate_buffer_android(
        mir_connection, size.width.as_int(), size.height.as_int(), native_pf, native_flags,
        buffer_available, this);
    std::unique_lock<std::mutex> lk(mut);
    cv.wait(lk, [&]{ return handle; });
    if (!mir_buffer_is_valid(handle))
    {
        mir_buffer_release(handle);
        BOOST_THROW_EXCEPTION(std::runtime_error("could not allocate MirBuffer"));
    }
}

mgn::HostBuffer::~HostBuffer()
{
    mir_buffer_release(handle);
}

void mgn::HostBuffer::sync(MirBufferAccess access, std::chrono::nanoseconds ns)
{
    if (fence_extensions && fence_extensions->wait_for_access)
        fence_extensions->wait_for_access(handle, access, ns.count());
}

MirBuffer* mgn::HostBuffer::client_handle() const
{
    return handle;
}

std::unique_ptr<mgn::GraphicsRegion> mgn::HostBuffer::get_graphics_region()
{
    return std::make_unique<mgn::GraphicsRegion>(handle);
}

geom::Size mgn::HostBuffer::size() const
{
    return { mir_buffer_get_width(handle), mir_buffer_get_height(handle) };
}

MirPixelFormat mgn::HostBuffer::format() const
{
    return mir_buffer_get_pixel_format(handle);
}

std::tuple<EGLenum, EGLClientBuffer, EGLint*> mgn::HostBuffer::egl_image_creation_hints() const
{
    EGLenum type;
    EGLClientBuffer client_buffer = nullptr;;
    EGLint* attrs = nullptr;
    // TODO: check return value
    mir_buffer_get_egl_image_parameters(handle, &type, &client_buffer, &attrs);
    
    return std::tuple<EGLenum, EGLClientBuffer, EGLint*>{type, client_buffer, attrs};
}

void mgn::HostBuffer::buffer_available(MirBuffer* buffer, void* context)
{
    auto host_buffer = static_cast<HostBuffer*>(context);
    host_buffer->available(buffer);
}

void mgn::HostBuffer::available(MirBuffer* buffer)
{
    std::unique_lock<std::mutex> lk(mut);
    if (!handle)
    {
        handle = buffer;
        cv.notify_all();
    }

    auto g = f;
    lk.unlock();
    if (g)
        g();
}

void mgn::HostBuffer::on_ownership_notification(std::function<void()> const& fn)
{
    std::unique_lock<std::mutex> lk(mut);
    f = fn;
}

MirBufferPackage* mgn::HostBuffer::package() const
{
   return mir_buffer_get_buffer_package(handle);
}

void mgn::HostBuffer::set_fence(mir::Fd fd)
{
    if (fence_extensions && fence_extensions->associate_fence)
        fence_extensions->associate_fence(handle, fd, mir_read_write);
}

mir::Fd mgn::HostBuffer::fence() const
{
    if (fence_extensions && fence_extensions->get_fence)
        return mir::Fd{mir::IntOwnedFd{fence_extensions->get_fence(handle)}};
    else
        return mir::Fd{mir::Fd::invalid};
}
