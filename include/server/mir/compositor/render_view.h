/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_GRAPHICS_RENDERABLE_COLLECTION_H
#define MIR_GRAPHICS_RENDERABLE_COLLECTION_H

#include "mir/geometry/forward.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Renderable;
}

namespace compositor
{
class FilterForRenderables
{
public:
    virtual ~FilterForRenderables() {}

    virtual bool operator()(graphics::Renderable& renderable) = 0;

protected:
    FilterForRenderables() = default;
    FilterForRenderables(const FilterForRenderables&) = delete;
    FilterForRenderables& operator=(const FilterForRenderables&) = delete;
};

class OperatorForRenderables
{
public:
    virtual ~OperatorForRenderables() {}

    virtual void operator()(graphics::Renderable& renderable) = 0;

protected:
    OperatorForRenderables() = default;
    OperatorForRenderables(const OperatorForRenderables&) = delete;
    OperatorForRenderables& operator=(const OperatorForRenderables&) = delete;

};

class RenderView
{
public:
    virtual ~RenderView() {}

    virtual void for_each_if(FilterForRenderables& filter, OperatorForRenderables& renderable_operator) = 0;

protected:
    RenderView() = default;

private:
    RenderView(RenderView const&) = delete;
    RenderView& operator=(RenderView const&) = delete;
};

}
}


#endif /* MIR_GRAPHICS_RENDERABLE_COLLECTION_H */
