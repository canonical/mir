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

namespace msh = mir::shell;
namespace mc = mir::compositor;

msh::ProxySurface::ProxySurface(std::weak_ptr<mir::surfaces::Surface> const& surface) :
    surface(surface),
    deleter([](std::weak_ptr<mir::surfaces::Surface> const&){})
{
}

msh::ProxySurface::ProxySurface(
    std::weak_ptr<mir::surfaces::Surface> const& surface,
    std::function<void(std::weak_ptr<mir::surfaces::Surface> const&)> const& deleter)
:
    surface(surface),
    deleter(deleter)
{
}

msh::ProxySurface::~ProxySurface()
{
    destroy();
}

void msh::ProxySurface::hide()
{
    if (auto const& s = surface.lock())
    {
        s->set_hidden(true);
    }
}

void msh::ProxySurface::show()
{
    if (auto const& s = surface.lock())
    {
        s->set_hidden(false);
    }
}

void msh::ProxySurface::destroy()
{
    deleter(surface);
}

void msh::ProxySurface::shutdown()
{
    if (auto const& s = surface.lock())
    {
        s->shutdown();
    }
}

mir::geometry::Size msh::ProxySurface::size() const
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

mir::geometry::PixelFormat msh::ProxySurface::pixel_format() const
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

void msh::ProxySurface::advance_client_buffer()
{
    if (auto const& s = surface.lock())
    {
        s->advance_client_buffer();
    }
}

std::shared_ptr<mc::Buffer> msh::ProxySurface::client_buffer() const
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
