/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_BUFFER_TEXTURE_BINDER_H_
#define MIR_GRAPHICS_MESA_BUFFER_TEXTURE_BINDER_H_

namespace mir
{
namespace graphics
{
namespace mesa
{

class BufferTextureBinder
{
public:
    virtual ~BufferTextureBinder() {}

    virtual void gl_bind_to_texture() = 0;

protected:
    BufferTextureBinder() = default;
    BufferTextureBinder(BufferTextureBinder const&) = delete;
    BufferTextureBinder& operator=(BufferTextureBinder const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_BUFFER_TEXTURE_BINDER_H_ */
