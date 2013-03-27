/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/surfaces/surface.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/surfaces/buffer_bundle.h"

#include <cassert>
#include <glm/gtc/matrix_transform.hpp>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

ms::Surface::Surface(
    const std::string& name,
    std::shared_ptr<BufferBundle> buffer_bundle,
    std::function<void()> const& change_callback) :
    surface_name(name),
    buffer_bundle(buffer_bundle),
    alpha_value(1.0f),
    is_hidden(false),
    notify_change(change_callback)
{
    // TODO(tvoss,kdub): Does a surface without a buffer_bundle make sense?
    assert(buffer_bundle);
    assert(change_callback);
}

void ms::Surface::shutdown()
{
    buffer_bundle->shutdown();
}

ms::Surface::~Surface()
{
    shutdown();
}

std::string const& ms::Surface::name() const
{
    return surface_name;
}

void ms::Surface::move_to(geometry::Point const& top_left)
{
    top_left_point = top_left;
    notify_change();
}

void ms::Surface::set_rotation(float degrees, glm::vec3 const& axis)
{
    transformation_matrix = glm::rotate(glm::mat4{1.0f}, degrees, axis);
    notify_change();
}

void ms::Surface::set_alpha(float alpha_v)
{
    alpha_value = alpha_v;
    notify_change();
}

geom::Point ms::Surface::top_left() const
{
    return top_left_point;
}

mir::geometry::Size ms::Surface::size() const
{
    return buffer_bundle->bundle_size();
}

std::shared_ptr<ms::GraphicRegion> ms::Surface::graphic_region() const
{
    return buffer_bundle->lock_back_buffer();
}

glm::mat4 ms::Surface::transformation() const
{
    return transformation_matrix;
}

float ms::Surface::alpha() const
{
    return alpha_value;
}

bool ms::Surface::hidden() const
{
    return is_hidden;
}

void ms::Surface::set_hidden(bool hide)
{
    is_hidden = hide;
    notify_change();
}

//note: not sure the surface should be aware of pixel format. might be something that the
//texture (which goes to compositor should be aware of though
//todo: kdub remove
geom::PixelFormat ms::Surface::pixel_format() const
{
    return buffer_bundle->get_bundle_pixel_format();
}

void ms::Surface::advance_client_buffer()
{
    /* we must hold a reference (client_buffer_resource) to the resource on behalf
       of the client until it is returned to us */
    /* todo: the surface shouldn't be holding onto the resource... the frontend should! */
    client_buffer_resource.reset();  // Release old client buffer
    client_buffer_resource = buffer_bundle->secure_client_buffer();
    notify_change();
}

std::shared_ptr<mc::Buffer> ms::Surface::client_buffer() const
{
    return client_buffer_resource;
}
