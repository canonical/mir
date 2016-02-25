/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
#include "presentation_chain.h"
#include "buffer.h"
#include "mir/require.h"
#include "mir/uncaught.h"
#include "mir/require.h"
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;

//private NBS api under development
void mir_presentation_chain_allocate_buffer(
    MirPresentationChain* chain, 
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage usage,
    mir_buffer_callback cb, void* context)
try
{
    mir::require(chain);
    chain->allocate_buffer(mir::geometry::Size{width, height}, format, usage, cb, context);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_buffer_release(MirBuffer* buffer) 
{
    delete reinterpret_cast<mcl::Buffer*>(buffer);
}

MirNativeFence* mir_buffer_get_fence(MirBuffer* b)
try
{
    mir::require(b);
    auto buffer = reinterpret_cast<mcl::Buffer*>(b);
    return buffer->get_fence();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void mir_buffer_associate_fence(MirBuffer* b, MirNativeFence* fence, MirBufferAccess access)
try
{
    mir::require(b);
    auto buffer = reinterpret_cast<mcl::Buffer*>(b);
    buffer->set_fence(fence, access);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

int mir_buffer_wait_for_access(MirBuffer* b, MirBufferAccess access, int timeout)
try
{
    mir::require(b);
    auto buffer = reinterpret_cast<mcl::Buffer*>(b);
    return buffer->wait_fence(access, std::chrono::nanoseconds(timeout));
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return -1;
}

MirNativeBuffer* mir_buffer_get_native_buffer(MirBuffer* b, MirBufferAccess access) 
try
{
    mir::require(b);
    auto buffer = reinterpret_cast<mcl::Buffer*>(b);
    if (!buffer->wait_fence(access, std::chrono::nanoseconds(-1)))
        BOOST_THROW_EXCEPTION(std::runtime_error("error accessing MirNativeBuffer"));
    return buffer->as_mir_native_buffer();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirGraphicsRegion mir_buffer_get_graphics_region(MirBuffer* b, MirBufferAccess access)
try
{
    mir::require(b);
    auto buffer = reinterpret_cast<mcl::Buffer*>(b);
    if (!buffer->wait_fence(access, std::chrono::nanoseconds(-1)))
        BOOST_THROW_EXCEPTION(std::runtime_error("error accessing MirNativeBuffer"));

    return buffer->map_region();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return MirGraphicsRegion { 0, 0, 0, mir_pixel_format_invalid, nullptr };
}
