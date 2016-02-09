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
#include "buffer.h"
#include "mir/uncaught.h"

namespace mcl = mir::client;

mcl::MirBufferStub::MirBufferStub(MirPresentationChain* stream, mir_buffer_callback cb, void* context) :
    stream(stream),
    cb(cb),
    context(context)
{
    ready();
}

void mcl::MirBufferStub::ready()
{
    if (cb)
        cb(stream, reinterpret_cast<MirBuffer*>(this), context);
}

//private NBS api under development
void mir_presentation_chain_allocate_buffer(
    MirPresentationChain* stream, 
    int, int,
    MirPixelFormat,
    MirBufferUsage,
    mir_buffer_callback cb, void* context)
{
    new mcl::MirBufferStub(stream, cb, context); 
}


void mir_buffer_release(MirBuffer* buffer) 
{
    delete reinterpret_cast<mcl::MirBufferStub*>(buffer);
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
