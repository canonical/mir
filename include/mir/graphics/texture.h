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
 * Authored by:
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_GRAPHICS_TEXTURE_H_
#define MIR_GRAPHICS_TEXTURE_H_

#include <memory>

namespace mir
{
namespace compositor
{
class Buffer;
}

namespace graphics
{

struct Texture
{
    explicit Texture(std::shared_ptr<mir::compositor::Buffer> const& buffer) :
        buffer(buffer) {}
private:
    std::shared_ptr<compositor::Buffer> buffer;
};

}
}

#endif // MIR_GRAPHICS_TEXTURE_H_
