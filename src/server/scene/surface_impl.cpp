/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "surface_impl.h"
#include "basic_surface.h"
#include "surface_builder.h"
#include "surface_ranker.h"

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mf = mir::frontend;

ms::SurfaceImpl::SurfaceImpl(
    std::shared_ptr<SurfaceBuilder> const& builder,
    std::shared_ptr<msh::SurfaceConfigurator> const& configurator,
    msh::SurfaceCreationParameters const& params,
    frontend::SurfaceId id,
    std::shared_ptr<mf::EventSink> const& event_sink)
  : builder(builder),
    surface(builder->create_surface(id, params, event_sink, configurator))
{
}

ms::SurfaceImpl::~SurfaceImpl() noexcept
{
    builder->destroy_surface(surface);
}

void ms::SurfaceImpl::hide()
{
    surface->hide();
}

void ms::SurfaceImpl::show()
{
    surface->show();
}

void ms::SurfaceImpl::force_requests_to_complete()
{
    surface->force_requests_to_complete();
}

mir::geometry::Size ms::SurfaceImpl::size() const
{
    return surface->size();
}

void ms::SurfaceImpl::move_to(geometry::Point const& p)
{
    surface->move_to(p);
}

mir::geometry::Point ms::SurfaceImpl::top_left() const
{
    return surface->top_left();
}

std::string ms::SurfaceImpl::name() const
{
    return surface->name();
}

MirPixelFormat ms::SurfaceImpl::pixel_format() const
{
    return surface->pixel_format();
}

void ms::SurfaceImpl::swap_buffers(mg::Buffer* old_buffer, std::function<void(mg::Buffer* new_buffer)> complete)
{
    surface->swap_buffers(old_buffer, complete);
}

void ms::SurfaceImpl::allow_framedropping(bool allow)
{
    surface->allow_framedropping(allow);
}

void ms::SurfaceImpl::with_most_recent_buffer_do(
    std::function<void(mg::Buffer&)> const& exec)
{
    surface->with_most_recent_buffer_do(exec);
}

bool ms::SurfaceImpl::supports_input() const
{
    return surface->supports_input();
}

int ms::SurfaceImpl::client_input_fd() const
{
    return surface->client_input_fd();
}

int ms::SurfaceImpl::configure(MirSurfaceAttrib attrib, int value)
{
    return surface->configure(attrib, value);
}

MirSurfaceType ms::SurfaceImpl::type() const
{
    return surface->type();
}

MirSurfaceState ms::SurfaceImpl::state() const
{
    return surface->state();
}

void ms::SurfaceImpl::take_input_focus(std::shared_ptr<msh::InputTargeter> const& targeter)
{
    surface->take_input_focus(targeter);
}

void ms::SurfaceImpl::set_input_region(std::vector<geom::Rectangle> const& region)
{
    surface->set_input_region(region);
}

void ms::SurfaceImpl::raise(std::shared_ptr<ms::SurfaceRanker> const& controller)
{
    // TODO - BasicSurface hasn't got the pointer to pass to controller
    controller->raise(surface);
}

void ms::SurfaceImpl::resize(geom::Size const& size)
{
    surface->resize(size);
}

void ms::SurfaceImpl::set_rotation(float degrees, glm::vec3 const& axis)
{
    surface->set_rotation(degrees, axis);
}

float ms::SurfaceImpl::alpha() const
{
    return surface->alpha();
}

void ms::SurfaceImpl::set_alpha(float alpha)
{
    surface->set_alpha(alpha);
}
