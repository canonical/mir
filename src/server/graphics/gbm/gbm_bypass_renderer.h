/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_GRAPHICS_GBM_BYPASS_RENDERER_H_
#define MIR_GRAPHICS_GBM_BYPASS_RENDERER_H_

#include "mir/graphics/renderer.h"
#include "mir/geometry/size.h"

namespace mir
{
namespace graphics
{
namespace gbm
{

class GBMBypassRenderer : public Renderer
{
public:
    /* From renderer */
    void render(std::function<void(std::shared_ptr<void> const&)> save_resource, Renderable& renderable);
    void clear();

    ~GBMBypassRenderer() noexcept {}

private:

};

}
}
}

#endif // MIR_GRAPHICS_GBM_BYPASS_RENDERER_H_
