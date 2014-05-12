/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_TEXTURE_H_
#define MIR_GRAPHICS_TEXTURE_H_

#include "mir/graphics/gl_program.h"
#include "mir/graphics/renderable.h"
#include <memory>

namespace mir
{
namespace graphics
{
class Texture
{
public:
    Texture();
    ~Texture();
    void gl_bind() const;

private:
    Texture(Texture const&) = delete;
    Texture& operator=(Texture const&) = delete;
    GLuint generate_id() const;
    GLuint const id;
};
}
}

#endif /* MIR_GRAPHICS_TEXTURE_H_ */
