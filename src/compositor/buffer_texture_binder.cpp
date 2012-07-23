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

#include "mir/compositor/buffer_texture_binder.h"
#include "mir/compositor/buffer.h"
#include "mir/graphics/texture.h"

#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

struct mc::BufferTextureBinder::Unlocker
{
    explicit Unlocker(BufferTextureBinder* binder) : binder(binder)
    {
        assert(binder);
        binder->lock_back_buffer();
    }

    void operator()(mg::Texture* texture) const
    {
        delete texture;
        binder->unlock_back_buffer();
    }

    BufferTextureBinder* binder;
};


std::shared_ptr<mg::Texture> mc::BufferTextureBinder::lock_and_bind_back_buffer()
{

    back_buffer()->lock();

    return std::shared_ptr<graphics::Texture>(
        back_buffer()->bind_to_texture(),
        Unlocker(this));
}
