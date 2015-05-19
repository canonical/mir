/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "surface_allocator.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/input/input_channel_factory.h"
#include "basic_surface.h"

namespace geom=mir::geometry;
namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace ms=mir::scene;
namespace msh=mir::shell;
namespace mi=mir::input;

static inline bool has_alpha(MirPixelFormat fmt)
{
    return (fmt == mir_pixel_format_abgr_8888) ||
           (fmt == mir_pixel_format_argb_8888);
}

ms::SurfaceAllocator::SurfaceAllocator(
    std::shared_ptr<input::InputChannelFactory> const& input_factory,
    std::shared_ptr<input::InputSender> const& input_sender,
    std::shared_ptr<mg::CursorImage> const& default_cursor_image,
    std::shared_ptr<SceneReport> const& report) :
    input_factory(input_factory),
    input_sender(input_sender),
    default_cursor_image(default_cursor_image),
    report(report)
{
}

std::shared_ptr<ms::Surface> ms::SurfaceAllocator::create_surface(
    std::shared_ptr<compositor::BufferStream> const& buffer_stream,
    SurfaceCreationParameters const& params)
{
    auto actual_size = geom::Rectangle{params.top_left, buffer_stream->stream_size()};

    bool nonrectangular = has_alpha(params.pixel_format);
    auto input_channel = input_factory->make_input_channel();
    auto const surface = std::make_shared<BasicSurface>(
        params.name,
        actual_size,
        params.parent,
        nonrectangular,
        buffer_stream,
        input_channel,
        input_sender,
        default_cursor_image,
        report);

    return surface;
}
