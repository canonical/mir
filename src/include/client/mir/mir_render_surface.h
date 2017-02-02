/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_CLIENT_MIR_RENDER_SURFACE_H
#define MIR_CLIENT_MIR_RENDER_SURFACE_H

#include "mir_toolkit/rs/mir_render_surface.h"
#include "mir/frontend/buffer_stream_id.h"
#include "mir/geometry/size.h"

class MirRenderSurface
{
public:
    virtual MirConnection* connection() const = 0;
    virtual mir::frontend::BufferStreamId stream_id() const = 0;
    virtual mir::geometry::Size size() const = 0;
    virtual void set_size(mir::geometry::Size) = 0;
    virtual bool valid() const = 0;
    virtual MirBufferStream* get_buffer_stream(
        int width, int height,
        MirPixelFormat format,
        MirBufferUsage buffer_usage) = 0;
    virtual MirPresentationChain* get_presentation_chain() = 0;
    virtual char const* get_error_message() const = 0;
    virtual ~MirRenderSurface() = default;
protected:
    MirRenderSurface(MirRenderSurface const&) = delete;
    MirRenderSurface& operator=(MirRenderSurface const&) = delete;
    MirRenderSurface() = default;
};

namespace mir
{
namespace client
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirRenderSurface* render_surface_lookup(void* key); 
#pragma GCC diagnostic pop
}
}

#endif /* MIR_CLIENT_MIR_RENDER_SURFACE_H */
