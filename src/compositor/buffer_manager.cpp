/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/compositor/buffer_manager.h"
#include "mir/graphics/framebuffer_backend.h"

#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::buffer_manager::buffer_manager(graphics::framebuffer_backend* framebuffer)
    :
    framebuffer(framebuffer)
{
    assert(framebuffer);
}


void mc::buffer_manager::bind_buffer_to_texture()
{
    framebuffer->render();
}
