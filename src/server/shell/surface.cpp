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

#include "surface.h"

#include "mir/surfaces/surface_stack_model.h"
#include "mir/input/input_channel.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace msh = mir::shell;
namespace mc = mir::compositor;
namespace mi = mir::input;

msh::Surface::Surface(std::weak_ptr<mir::surfaces::Surface> const& surface,
		      std::shared_ptr<input::InputChannel> const& input_channel)
  : surface(surface),
    input_channel(input_channel),
    deleter([](std::weak_ptr<mir::surfaces::Surface> const&){})
{
}

msh::Surface::Surface(
    std::weak_ptr<mir::surfaces::Surface> const& surface,
    std::shared_ptr<input::InputChannel> const& input_channel,
    std::function<void(std::weak_ptr<mir::surfaces::Surface> const&)> const& deleter)
:
    surface(surface),
    input_channel(input_channel),
    deleter(deleter)
{
}

msh::Surface::~Surface()
{
    destroy();
}

void msh::Surface::hide()
{
    if (auto const& s = surface.lock())
    {
        s->set_hidden(true);
    }
}

void msh::Surface::show()
{
    if (auto const& s = surface.lock())
    {
        s->set_hidden(false);
    }
}

void msh::Surface::destroy()
{
    deleter(surface);
}

void msh::Surface::shutdown()
{
    if (auto const& s = surface.lock())
    {
        s->shutdown();
    }
}

mir::geometry::Size msh::Surface::size() const
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

std::string msh::Surface::name() const
{
    if (auto const& s = surface.lock())
    {
        return s->name();
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface"));
    }
}

mir::geometry::PixelFormat msh::Surface::pixel_format() const
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

void msh::Surface::advance_client_buffer()
{
    if (auto const& s = surface.lock())
    {
        s->advance_client_buffer();
    }
}

std::shared_ptr<mc::Buffer> msh::Surface::client_buffer() const
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

bool msh::Surface::supports_input() const
{
    if (input_channel)
        return true;
    return false;
}

int msh::Surface::client_input_fd() const
{
    if (!supports_input())
        BOOST_THROW_EXCEPTION(std::logic_error("Surface does not support input"));
    return input_channel->client_fd();
}

int msh::Surface::server_input_fd() const
{
    if (!supports_input())
        BOOST_THROW_EXCEPTION(std::logic_error("Surface does not support input"));
    return input_channel->server_fd();
}
