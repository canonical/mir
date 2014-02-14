/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_COMPOSITOR_RENDERER_H_
#define MIR_COMPOSITOR_RENDERER_H_

namespace mir
{
namespace graphics
{
class Buffer;
}
namespace compositor
{
class CompositingCriteria;

class Renderer
{
public:
    virtual ~Renderer() = default;

    virtual void begin(float rotation = 0.0f) const = 0;
    virtual void render(CompositingCriteria const& info, graphics::Buffer& buffer) const = 0;
    virtual void end() const = 0;

    virtual void suspend() = 0; // called when begin/render/end skipped

protected:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
};

}
}

#endif // MIR_COMPOSITOR_RENDERER_H_
