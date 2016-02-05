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

//private NBS api under development
void mir_presentation_chain_allocate_buffer(
    MirPresentationChain*, 
    int, int,
    MirPixelFormat,
    MirBufferUsage,
    mir_buffer_callback cb, void* context)
{
    int fake_id = 3;
    new mcl::Buffer(cb, context, fake_id, nullptr); 
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
