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

namespace mir
{

namespace surfaces
{
class SurfacesToRender;
}

namespace graphics
{
class Texture;
}

namespace compositor
{


class BufferTextureBinder
{
public:

    virtual void bind_buffer_to_texture(surfaces::SurfacesToRender const& ) = 0;

protected:
    BufferTextureBinder() = default;
    ~BufferTextureBinder() = default;
private:
    BufferTextureBinder(BufferTextureBinder const&) = delete;
    BufferTextureBinder& operator=(BufferTextureBinder const&) = delete;
};
}
}


#endif /* MIR_COMPOSITOR_BUFFER_TEXTURE_BINDER_H_ */
