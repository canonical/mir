/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/surfaces/surface_allocator.h"
#include "mir/surfaces/buffer_stream_factory.h"
#include "mir/surfaces/buffer_stream.h"
#include "mir/graphics/surface_state.h"
#include "mir/input/surface_data_storage.h"
#include "mir/input/input_channel_factory.h"
#include "mir/surfaces/surface_data_storage.h"
#include "mir/surfaces/surface.h"

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace ms=mir::surfaces;
namespace msh=mir::shell;
namespace mi=mir::input;

ms::SurfaceAllocator::SurfaceAllocator(
    std::shared_ptr<BufferStreamFactory> const& stream_factory,
    std::shared_ptr<input::InputChannelFactory> const& input_factory)
    : buffer_stream_factory(stream_factory),
      input_factory(input_factory) 
{
}

std::shared_ptr<ms::Surface> ms::SurfaceAllocator::create_surface(
    msh::SurfaceCreationParameters const& params, std::function<void()> const& change_callback)
{
    mc::BufferProperties buffer_properties{params.size,
                                           params.pixel_format,
                                           params.buffer_usage};
    auto buffer_stream = buffer_stream_factory->create_buffer_stream(buffer_properties);
    auto basic_info = std::make_shared<ms::SurfaceDataStorage>(params.name,
                                                         params.top_left,
                                                         buffer_stream->stream_size());
    auto graphics_info = std::make_shared<mg::SurfaceState>(basic_info, change_callback);
    auto input_info = std::make_shared<mi::SurfaceDataStorage>(basic_info);
    auto input_channel = input_factory->make_input_channel();
    return std::make_shared<ms::Surface>(
                        basic_info,
                        graphics_info, buffer_stream,
                        input_info, input_channel,
                        change_callback);
}
