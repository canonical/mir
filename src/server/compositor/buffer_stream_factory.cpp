/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "buffer_stream_factory.h"
#include "buffer_stream_surfaces.h"
#include "mir/graphics/buffer_properties.h"
#include "buffer_queue.h"
#include "stream.h"
#include "buffer_map.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"

#include <cassert>
#include <memory>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mf = mir::frontend;

mc::BufferStreamFactory::BufferStreamFactory(
    std::shared_ptr<mg::GraphicBufferAllocator> const& gralloc,
    std::shared_ptr<mc::FrameDroppingPolicyFactory> const& policy_factory,
    unsigned int nbuffers) :
    gralloc(gralloc),
    policy_factory{policy_factory},
    nbuffers(nbuffers)
{
    assert(gralloc);
    assert(policy_factory);
    if (nbuffers == 1)
        throw std::logic_error("nbuffers cannot be 1");
}


std::shared_ptr<mc::BufferStream> mc::BufferStreamFactory::create_buffer_stream(
    mf::BufferStreamId id, std::shared_ptr<mf::ClientBuffers> const& buffers,
    mg::BufferProperties const& buffer_properties)
{
    return create_buffer_stream(id, buffers, nbuffers, buffer_properties);
}

std::shared_ptr<mc::BufferStream> mc::BufferStreamFactory::create_buffer_stream(
    mf::BufferStreamId, std::shared_ptr<mf::ClientBuffers> const& buffers,
    int nbuffers, mg::BufferProperties const& buffer_properties)
{
    if (nbuffers == 0)
    {
        return std::make_shared<mc::Stream>(
            *policy_factory,
            buffers,
            buffer_properties.size, buffer_properties.format);
    }
    else
    {
        auto switching_bundle = std::make_shared<mc::BufferQueue>(
            nbuffers, gralloc, buffer_properties, *policy_factory);
        return std::make_shared<mc::BufferStreamSurfaces>(switching_bundle);
    }
}

std::shared_ptr<mf::ClientBuffers> mc::BufferStreamFactory::create_buffer_map(
        std::shared_ptr<mf::BufferSink> const& sink)
{
    return std::make_shared<mc::BufferMap>(sink, gralloc);
}
