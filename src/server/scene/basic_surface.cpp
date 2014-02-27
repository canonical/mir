/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "basic_surface.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/input/input_channel.h"
#include "mir/graphics/buffer.h"

#include "mir/scene/scene_report.h"

#include <glm/gtc/matrix_transform.hpp>

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::BasicSurface::BasicSurface(
    std::string const& name,
    geometry::Rectangle rect,
    std::function<void()> change_cb,
    bool nonrectangular,
    std::shared_ptr<mc::BufferStream> const& buffer_stream,
    std::shared_ptr<input::InputChannel> const& input_channel,
    std::shared_ptr<SceneReport> const& report) :
    notify_change(change_cb),
    surface_name(name),
    surface_rect(rect),
    transformation_dirty(true),
    surface_alpha(1.0f),
    first_frame_posted(false),
    hidden(false),
    nonrectangular(nonrectangular),
    input_rectangles{surface_rect},
    surface_buffer_stream(buffer_stream),
    server_input_channel(input_channel),
    report(report)
{
    report->surface_created(this, surface_name);
}

void ms::BasicSurface::force_requests_to_complete()
{
    surface_buffer_stream->force_requests_to_complete();
}

ms::BasicSurface::~BasicSurface() noexcept
{
    report->surface_deleted(this, surface_name);
}

std::shared_ptr<mc::BufferStream> ms::BasicSurface::buffer_stream() const
{
    return surface_buffer_stream;
}

std::string const& ms::BasicSurface::name() const
{
    return surface_name;
}

void ms::BasicSurface::move_to(geometry::Point const& top_left)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_rect.top_left = top_left;
        transformation_dirty = true;
    }
    notify_change();
}

float ms::BasicSurface::alpha() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_alpha;
}

void ms::BasicSurface::set_hidden(bool hide)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        hidden = hide;
    }
    notify_change();
}

mir::geometry::Size ms::BasicSurface::size() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.size;
}

MirPixelFormat ms::BasicSurface::pixel_format() const
{
    return surface_buffer_stream->get_stream_pixel_format();
}

void ms::BasicSurface::swap_buffers(mg::Buffer* old_buffer, std::function<void(mg::Buffer* new_buffer)> complete)
{
    bool const posting{!!old_buffer};

    surface_buffer_stream->swap_client_buffers(old_buffer, complete);

    if (posting)
    {
        {
            std::unique_lock<std::mutex> lk(guard);
            first_frame_posted = true;
        }
        notify_change();
    }
}

void ms::BasicSurface::allow_framedropping(bool allow)
{
    surface_buffer_stream->allow_framedropping(allow);
}

std::shared_ptr<mg::Buffer> ms::BasicSurface::snapshot_buffer() const
{
    return surface_buffer_stream->lock_snapshot_buffer();
}

bool ms::BasicSurface::supports_input() const
{
    if (server_input_channel)
        return true;
    return false;
}

int ms::BasicSurface::client_input_fd() const
{
    if (!supports_input())
        BOOST_THROW_EXCEPTION(std::logic_error("Surface does not support input"));
    return server_input_channel->client_fd();
}

std::shared_ptr<mi::InputChannel> ms::BasicSurface::input_channel() const
{
    return server_input_channel;
}

void ms::BasicSurface::set_input_region(std::vector<geom::Rectangle> const& input_rectangles)
{
    std::unique_lock<std::mutex> lock(guard);
    this->input_rectangles = input_rectangles;
}

bool ms::BasicSurface::resize(geom::Size const& size)
{
    if (size == this->size())
        return false;

    if (size.width <= geom::Width{0} || size.height <= geom::Height{0})
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Impossible resize requested"));
    }
    /*
     * Other combinations may still be invalid (like dimensions too big or
     * insufficient resources), but those are runtime and platform-specific, so
     * not predictable here. Such critical exceptions would arise from
     * the platform buffer allocator as a runtime_error via:
     */
    surface_buffer_stream->resize(size);

    // Now the buffer stream has successfully resized, update the state second;
    {
        std::unique_lock<std::mutex> lock(guard);
        surface_rect.size = size;
        transformation_dirty = true;
    }
    notify_change();

    return true;
}

geom::Point ms::BasicSurface::top_left() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.top_left;
}

bool ms::BasicSurface::contains(geom::Point const& point) const
{
    std::unique_lock<std::mutex> lock(guard);
    if (hidden)
        return false;

    for (auto const& rectangle : input_rectangles)
    {
        if (rectangle.contains(point))
        {
            return true;
        }
    }
    return false;
}

void ms::BasicSurface::frame_posted()
{
    {
        std::unique_lock<std::mutex> lk(guard);
        first_frame_posted = true;
    }
    notify_change();
}

void ms::BasicSurface::set_alpha(float alpha)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_alpha = alpha;
    }
    notify_change();
}


void ms::BasicSurface::set_rotation(float degrees, glm::vec3 const& axis)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        rotation_matrix = glm::rotate(glm::mat4{1.0f}, degrees, axis);
        transformation_dirty = true;
    }
    notify_change();
}

glm::mat4 const& ms::BasicSurface::transformation() const
{
    std::unique_lock<std::mutex> lk(guard);

    auto surface_size = surface_rect.size;
    auto surface_top_left = surface_rect.top_left;
    if (transformation_dirty || transformation_size != surface_size)
    {
        const glm::vec3 top_left_vec{surface_top_left.x.as_int(),
                                     surface_top_left.y.as_int(),
                                     0.0f};
        const glm::vec3 size_vec{surface_size.width.as_uint32_t(),
                                 surface_size.height.as_uint32_t(),
                                 0.0f};

        /* Get the center of the renderable's area */
        const glm::vec3 center_vec{top_left_vec + 0.5f * size_vec};

        /*
         * Every renderable is drawn using a 1x1 quad centered at 0,0.
         * We need to transform and scale that quad to get to its final position
         * and size.
         *
         * 1. We scale the quad vertices (from 1x1 to wxh)
         * 2. We move the quad to its final position. Note that because the quad
         *    is centered at (0,0), we need to translate by center_vec, not
         *    top_left_vec.
         */
        glm::mat4 pos_size_matrix;
        pos_size_matrix = glm::translate(pos_size_matrix, center_vec);
        pos_size_matrix = glm::scale(pos_size_matrix, size_vec);

        // Rotate, then scale, then translate
        transformation_matrix = pos_size_matrix * rotation_matrix;
        transformation_size = surface_size;
        transformation_dirty = false;
    }

    return transformation_matrix;
}

bool ms::BasicSurface::should_be_rendered_in(geom::Rectangle const& rect) const
{
    std::unique_lock<std::mutex> lk(guard);

    if (hidden || !first_frame_posted)
        return false;

    return rect.overlaps(surface_rect);
}

bool ms::BasicSurface::shaped() const
{
    return nonrectangular;
}
