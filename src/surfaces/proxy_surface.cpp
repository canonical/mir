/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "proxy_surface.h"

#include "mir/surfaces/surface_stack_model.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace ms = mir::surfaces;
namespace mc = mir::compositor;

ms::BasicProxySurface::BasicProxySurface(std::weak_ptr<mir::surfaces::Surface> const& surface) :
    surface(surface)
{
}

void ms::BasicProxySurface::hide()
{
    if (auto const& s = surface.lock())
    {
        s->set_hidden(true);
    }
}

void ms::BasicProxySurface::show()
{
    if (auto const& s = surface.lock())
    {
        s->set_hidden(false);
    }
}

void ms::BasicProxySurface::destroy()
{
}

void ms::BasicProxySurface::shutdown()
{
    if (auto const& s = surface.lock())
    {
        s->shutdown();
    }
}

mir::geometry::Size ms::BasicProxySurface::size() const
{
    if (auto const& s = surface.lock())
    {
        return s->size();
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface"));
    }
}

mir::geometry::PixelFormat ms::BasicProxySurface::pixel_format() const
{
    if (auto const& s = surface.lock())
    {
        return s->pixel_format();
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface"));
    }
}

void ms::BasicProxySurface::advance_client_buffer()
{
    if (auto const& s = surface.lock())
    {
        s->advance_client_buffer();
    }
}

std::shared_ptr<mc::Buffer> ms::BasicProxySurface::client_buffer() const
{
    if (auto const& s = surface.lock())
    {
        return s->client_buffer();
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface"));
    }
}

void ms::BasicProxySurface::destroy_surface(SurfaceStackModel* const surface_stack) const
{
    surface_stack->destroy_surface(surface);
}


ms::ProxySurface::ProxySurface(
        SurfaceStackModel* const surface_stack_,
        shell::SurfaceCreationParameters const& params) :
    BasicProxySurface(surface_stack_->create_surface(params)),
    surface_stack(surface_stack_)
{
}

void ms::ProxySurface::destroy()
{
    destroy_surface(surface_stack);
}

ms::ProxySurface::~ProxySurface()
{
    destroy();
}
