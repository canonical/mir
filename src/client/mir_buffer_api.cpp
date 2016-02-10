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
#include "mir/uncaught.h"
#include "mir/client_buffer.h"
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;

//private NBS api under development
void mir_presentation_chain_allocate_buffer(
    MirPresentationChain* client_chain, 
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage usage,
    mir_buffer_callback cb, void* context)
try
{
    auto chain = reinterpret_cast<mcl::PresentationChain*>(client_chain);
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

MirNativeFence* mir_buffer_get_fence(MirBuffer*)
{
    return nullptr;
}

void mir_buffer_associate_fence(MirBuffer*, MirNativeFence*, MirBufferAccess)
{
}

int mir_buffer_wait_fence(MirBuffer*, MirBufferAccess, int)
{
    return 0;
}

MirNativeBuffer* mir_buffer_get_native_buffer(MirBuffer*, MirBufferAccess) 
{
    return nullptr;
}

MirGraphicsRegion* mir_buffer_acquire_region(MirBuffer*, MirBufferAccess)
{
    return nullptr;
}

void mir_buffer_release_region(MirGraphicsRegion*) 
{
}
