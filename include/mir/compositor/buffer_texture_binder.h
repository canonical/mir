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

#ifndef MIR_COMPOSITOR_BUFFER_TEXTURE_BINDER_H_
#define MIR_COMPOSITOR_BUFFER_TEXTURE_BINDER_H_

#include <memory>

namespace mir
{
namespace graphics
{
class Texture;
}

namespace compositor
{
class Buffer;

class BufferTextureBinder
{
public:
    virtual std::shared_ptr<graphics::Texture> lock_and_bind_back_buffer() = 0;
    virtual void unlock_back_buffer() = 0;

protected:
    BufferTextureBinder() = default;
    ~BufferTextureBinder() {}

private:
    class Unlocker;
    BufferTextureBinder(BufferTextureBinder const&) = delete;
    BufferTextureBinder& operator=(BufferTextureBinder const&) = delete;
};
}
}


#endif /* MIR_COMPOSITOR_BUFFER_TEXTURE_BINDER_H_ */
