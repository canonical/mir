/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_INITIAL_RENDER_H
#define MIR_GRAPHICS_INITIAL_RENDER_H

#include <vector>
#include <memory>

namespace mir
{
namespace graphics
{

class Renderable;

/// A Display is able to specify an initial render that it wants to show to the user
/// (such as the plymouth buffer for a fade-in effect from startup splash to compostior).
class InitialRender
{
public:
    virtual ~InitialRender() = default;
    virtual auto get_renderables() const -> std::vector<std::shared_ptr<Renderable>> = 0;

protected:
    InitialRender() = default;
    InitialRender(InitialRender const&) = delete;
    InitialRender& operator=(InitialRender const&) = delete;
};

/// Manages the lifetime of InitialRender objects
class InitialRenderManager
{
public:
    virtual ~InitialRenderManager() = default;
    virtual void add_initial_render(std::shared_ptr<InitialRender> const&) = 0;
};

}
}

#endif
