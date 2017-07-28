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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_toolkit/mir_buffer_private.h"
#include "presentation_chain.h"
#include "mir_connection.h"
#include "buffer.h"
#include "mir/client/client_buffer.h"
#include "mir/require.h"
#include "mir/uncaught.h"
#include "mir/require.h"
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;

//private NBS api under development
void mir_connection_allocate_buffer(
    MirConnection* connection, 
    int width, int height,
    MirPixelFormat format,
    MirBufferCallback cb, void* context)
try
{
    mir::require(connection);
    connection->allocate_buffer(mir::geometry::Size{width, height}, format, cb, context);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirBuffer* mir_connection_allocate_buffer_sync(
    MirConnection* connection, 
    int width, int height,
    MirPixelFormat format)
try
{
    mir::require(connection);

    struct BufferInfo
    {
        MirBuffer* buffer = nullptr;
        std::mutex mutex;
        std::condition_variable cv;
    } info;
    connection->allocate_buffer(mir::geometry::Size{width, height}, format,
        [](MirBuffer* buffer, void* c)
        {
            mir::require(buffer);
            mir::require(c);
            auto context = reinterpret_cast<BufferInfo*>(c);
            std::unique_lock<decltype(context->mutex)> lk(context->mutex);
            context->buffer = buffer;
            context->cv.notify_all();
        }, &info);

    std::unique_lock<decltype(info.mutex)> lk(info.mutex);
    info.cv.wait(lk, [&]{ return info.buffer; });
    return info.buffer;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void ignore(MirBuffer*, void*){}
void mir_buffer_release(MirBuffer* b) 
try
{
    mir::require(b);
    auto buffer = reinterpret_cast<mcl::MirBuffer*>(b);
    buffer->set_callback(ignore, nullptr);
    auto connection = buffer->allocating_connection();
    connection->release_buffer(buffer);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

bool mir_buffer_map(MirBuffer* b, MirGraphicsRegion* region, MirBufferLayout* layout)
try
{
    mir::require(b);
    mir::require(region);
    mir::require(layout);
    auto buffer = reinterpret_cast<mcl::MirBuffer*>(b);
    *layout = mir_buffer_layout_linear;
    *region = buffer->map_region();
    return true;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    *region = MirGraphicsRegion { 0, 0, 0, mir_pixel_format_invalid, nullptr };
    *layout = mir_buffer_layout_unknown;
    return false;
}

void mir_buffer_unmap(MirBuffer* b)
try
{
    mir::require(b);
    auto buffer = reinterpret_cast<mcl::MirBuffer*>(b);
    buffer->unmap_region();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

unsigned int mir_buffer_get_width(MirBuffer const* b)
try
{
    mir::require(b);
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    return buffer->size().width.as_uint32_t();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return 0;
}

unsigned int mir_buffer_get_height(MirBuffer const* b)
try
{
    mir::require(b);
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    return buffer->size().height.as_uint32_t();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return 0;
}

MirPixelFormat mir_buffer_get_pixel_format(MirBuffer const* b)
try
{
    mir::require(b);
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    return buffer->pixel_format();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return mir_pixel_format_invalid;
}

bool mir_buffer_is_valid(MirBuffer const* b)
try
{
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    return buffer->valid();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}

char const *mir_buffer_get_error_message(MirBuffer const* b)
try
{
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    return buffer->error_message();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return "MirBuffer: unknown error";
}

MirBufferPackage* mir_buffer_get_buffer_package(MirBuffer* b)
try
{
    mir::require(b);
    auto buffer = reinterpret_cast<mcl::Buffer*>(b);
    return buffer->client_buffer()->package();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

bool mir_buffer_get_egl_image_parameters(
    MirBuffer* b, EGLenum* type, EGLClientBuffer* client_buffer, EGLint** attr)
try
{
    mir::require(b);
    mir::require(type);
    mir::require(client_buffer);
    mir::require(attr);
    auto buffer = reinterpret_cast<mcl::Buffer*>(b);
    buffer->client_buffer()->egl_image_creation_parameters(type, client_buffer, attr);
    return true;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}
