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

#include "mir/compositor/compositor.h"
#include "mir/compositor/buffer_manager.h"

#include <cassert>

namespace mc = mir::compositor;


mc::compositor::compositor(
    surfaces::scenegraph* scenegraph,
    buffer_texture_binder* buffermanager)
    :
    scenegraph(scenegraph),
    buffermanager(buffermanager)
{
    assert(buffermanager);
}

void mc::compositor::render(graphics::display*)
{
    buffermanager->bind_buffer_to_texture();
}
