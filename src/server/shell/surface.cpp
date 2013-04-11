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

#include "mir/shell/surface.h"
#include "mir/shell/surface_builder.h"
#include "mir/input/input_channel.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace msh = mir::shell;
namespace mc = mir::compositor;
namespace mi = mir::input;

msh::Surface::Surface(
    std::shared_ptr<SurfaceBuilder> const& builder,
    frontend::SurfaceCreationParameters const& params,
    std::shared_ptr<input::InputChannel> const& input_channel)
  : builder(builder),
    input_channel(input_channel),
    surface(builder->create_surface(params)),
    type_value(mir_surface_type_normal)
{
}

msh::Surface::~Surface()
{
    if (surface.lock())
    {
        destroy();
    }
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
    builder->destroy_surface(surface);
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

int msh::Surface::configure(MirSurfaceAttrib attrib, int value)
{
    int result = 0;

    /*
     * TODO: In future, query the shell implementation for the subset of
     *       attributes/types it implements.
     */
    switch (attrib)
    {
    case mir_surface_attrib_type:
        if (!set_type(static_cast<MirSurfaceType>(value)))
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                                   "type."));
        result = type();
        break;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                               "attribute."));
        break;
    }

    return result;
}

MirSurfaceType msh::Surface::type() const
{
    return type_value;
}

bool msh::Surface::set_type(MirSurfaceType t)
{
    bool valid = false;

    if (t >= 0 && t < mir_surface_type_arraysize_)
    {
        type_value = t;
        valid = true;
    }

    return valid;
}
