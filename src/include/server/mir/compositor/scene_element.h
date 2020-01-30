/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_SCENE_ELEMENT_H_
#define MIR_COMPOSITOR_SCENE_ELEMENT_H_

#include <memory>

namespace mir
{
namespace graphics
{
class Renderable;
}
namespace compositor
{
class Decoration;
class SceneElement
{
public:
    virtual ~SceneElement() = default;

    virtual std::shared_ptr<graphics::Renderable> renderable() const = 0;
    virtual void rendered() = 0;
    virtual void occluded() = 0;

protected:
    SceneElement() = default;
    SceneElement(SceneElement const&) = delete;
    SceneElement& operator=(SceneElement const&) = delete;
};

}
}

#endif // MIR_COMPOSITOR_SCENE_ELEMENT_H_
