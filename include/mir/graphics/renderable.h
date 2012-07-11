/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_GRAPHICS_RENDERABLE_H_
#define MIR_GRAPHICS_RENDERABLE_H_

namespace mir
{
namespace graphics
{

// The interface by which a Renderer talks to, e.g., a surface.
class Renderable
{
public:
    virtual ~Renderable() {}

protected:
    Renderable() = default;
    Renderable(const Renderable&) = delete;
    Renderable& operator=(const Renderable&) = delete;
};

}
}

#endif // MIR_GRAPHICS_RENDERABLE_H_
